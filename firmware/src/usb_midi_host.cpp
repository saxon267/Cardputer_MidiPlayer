#include "usb_midi_host.h"

#if USB_MIDI_HOST
#include <Arduino.h>
#include <esp_intr_alloc.h>
#include <usb/usb_helpers.h>
#include <usb/usb_host.h>

UsbMidiHost usbMidi;

bool UsbMidiHost::begin()
{
    usb_host_config_t hostConfig{};
    hostConfig.skip_phy_setup = false;
    hostConfig.intr_flags = ESP_INTR_FLAG_LEVEL1;
    if (usb_host_install(&hostConfig) != ESP_OK) return false;

    usb_host_client_config_t clientConfig{};
    clientConfig.is_synchronous = false;
    clientConfig.max_num_event_msg = 8;
    clientConfig.async.client_event_callback = clientCb;
    clientConfig.async.callback_arg = this;
    if (usb_host_client_register(&clientConfig, &_client) != ESP_OK) {
        usb_host_uninstall();
        _client = nullptr;
        return false;
    }
    return true;
}

void UsbMidiHost::clientCb(const usb_host_client_event_msg_t* msg, void* arg)
{
    if (!msg || !arg) return;
    auto* self = static_cast<UsbMidiHost*>(arg);
    if (msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        if (!self->_dev) self->openDevice(msg->new_dev.address);
    } else if (msg->event == USB_HOST_CLIENT_EVENT_DEV_GONE) {
        if (self->_dev && msg->dev_gone.dev_hdl == self->_dev) {
            self->_seen = false;
            self->closeDevice();
        }
    }
}

void UsbMidiHost::transferCb(usb_transfer_t* t)
{
    if (t && t->context) static_cast<UsbMidiHost*>(t->context)->_inFlight = false;
}

void UsbMidiHost::poll()
{
    if (!_client) return;
    usb_host_client_handle_events(_client, 0);
    uint32_t flags = 0;
    usb_host_lib_handle_events(0, &flags);
    submitNext();
}

void UsbMidiHost::openDevice(uint8_t address)
{
    if (usb_host_device_open(_client, address, &_dev) != ESP_OK) return;
    _seen = true;
    _midiFound = false;
    _head = _tail = 0;

    const usb_device_desc_t* dd = nullptr;
    if (usb_host_get_device_descriptor(_dev, &dd) == ESP_OK && dd) {
        _vid = dd->idVendor;
        _pid = dd->idProduct;
    }

    const usb_config_desc_t* cfg = nullptr;
    if (usb_host_get_active_config_descriptor(_dev, &cfg) != ESP_OK || !cfg) {
        closeDevice();
        return;
    }
    parseConfig(cfg);
    if (!_connected) closeDevice();
}

void UsbMidiHost::parseConfig(const usb_config_desc_t* cfg)
{
    const uint8_t* p = cfg->val;
    const uint8_t* end = p + cfg->wTotalLength;
    bool inMidi = false;
    uint8_t iface = 0, alt = 0;

    while (p + 2 <= end && p[0] >= 2 && p + p[0] <= end) {
        if (p[1] == USB_B_DESCRIPTOR_TYPE_INTERFACE) {
            const auto* id = reinterpret_cast<const usb_intf_desc_t*>(p);
            if (p[0] >= sizeof(usb_intf_desc_t)) {
                inMidi = (id->bInterfaceClass == 1 && id->bInterfaceSubClass == 3);
                iface = id->bInterfaceNumber;
                alt = id->bAlternateSetting;
            }
        } else if (p[1] == USB_B_DESCRIPTOR_TYPE_ENDPOINT && inMidi) {
            if (p[0] < sizeof(usb_ep_desc_t)) break;
            const auto* ep = reinterpret_cast<const usb_ep_desc_t*>(p);
            const bool isOut = (ep->bEndpointAddress & 0x80) == 0;
            const uint8_t type = ep->bmAttributes & USB_BM_ATTRIBUTES_XFERTYPE_MASK;
            const bool bulk = type == USB_BM_ATTRIBUTES_XFER_BULK || type == USB_BM_ATTRIBUTES_XFER_INT;
            if (isOut && bulk && ep->wMaxPacketSize) {
                if (usb_host_interface_claim(_client, _dev, iface, alt) == ESP_OK) {
                    if (usb_host_transfer_alloc(ep->wMaxPacketSize, 0, &_out) == ESP_OK) {
                        _iface = iface;
                        _alt = alt;
                        _ep = ep->bEndpointAddress;
                        _pkt = ep->wMaxPacketSize > MAX_XFER ? MAX_XFER : ep->wMaxPacketSize;
                        _out->device_handle = _dev;
                        _out->bEndpointAddress = _ep;
                        _out->callback = transferCb;
                        _out->context = this;
                        _midiFound = true;
                        _connected = true;
                        fire(true);
                        return;
                    }
                    usb_host_interface_release(_client, _dev, iface);
                }
            }
        }
        p += p[0];
    }
}

void UsbMidiHost::closeDevice()
{
    const bool was = _connected;
    _connected = false;
    _inFlight = false;
    if (_out) {
        usb_host_transfer_free(_out);
        _out = nullptr;
    }
    if (_dev) {
        if (_midiFound) usb_host_interface_release(_client, _dev, _iface);
        usb_host_device_close(_client, _dev);
        _dev = nullptr;
    }
    _midiFound = false;
    _ep = 0;
    _head = _tail = 0;
    if (was) fire(false);
}

void UsbMidiHost::fire(bool c)
{
    if (_cb) _cb(c, _cbCtx);
}

void UsbMidiHost::push(const uint8_t pkt[4])
{
    uint16_t next = (_tail + 1) % QSIZE;
    if (next == _head) return;  // full: drop oldest-safe
    memcpy(_q[_tail], pkt, 4);
    _tail = next;
}

bool UsbMidiHost::pop(uint8_t pkt[4])
{
    if (_head == _tail) return false;
    memcpy(pkt, _q[_head], 4);
    _head = (_head + 1) % QSIZE;
    return true;
}

void UsbMidiHost::submitNext()
{
    if (!_connected || !_out || _inFlight) return;

    int max = _pkt & ~3;
    if (max < 4) return;
    int n = 0;
    uint8_t pkt[4];
    while (n + 4 <= max && pop(pkt)) {
        memcpy(_out->data_buffer + n, pkt, 4);
        n += 4;
    }
    if (!n) return;

    _out->num_bytes = n;
    _inFlight = true;
    if (usb_host_transfer_submit(_out) != ESP_OK) _inFlight = false;
}

void UsbMidiHost::send2(uint8_t cin, uint8_t status, uint8_t d1, uint8_t d2)
{
    uint8_t p[4] = {cin, status, d1, d2};
    push(p);
}

void UsbMidiHost::noteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
    send2(0x09, (uint8_t)(0x90 | (ch & 0x0F)), note & 0x7F, vel & 0x7F);
}

void UsbMidiHost::noteOff(uint8_t ch, uint8_t note)
{
    send2(0x08, (uint8_t)(0x80 | (ch & 0x0F)), note & 0x7F, 0);
}

void UsbMidiHost::controlChange(uint8_t ch, uint8_t cc, uint8_t value)
{
    send2(0x0B, (uint8_t)(0xB0 | (ch & 0x0F)), cc & 0x7F, value & 0x7F);
}

void UsbMidiHost::panic()
{
    for (uint8_t ch = 0; ch < 16; ++ch) {
        controlChange(ch, 64, 0);
        controlChange(ch, 123, 0);
        controlChange(ch, 120, 0);
    }
}
#endif
