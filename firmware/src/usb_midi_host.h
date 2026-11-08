#pragma once
#include <stdint.h>

#if USB_MIDI_HOST
#include <usb/usb_host.h>

class UsbMidiHost {
public:
    using ConnectCallback = void (*)(bool connected, void* context);

    bool begin();
    void poll();
    bool connected() const { return _connected; }
    bool midiFound() const { return _midiFound; }
    bool deviceSeen() const { return _seen; }
    uint16_t vid() const { return _vid; }
    uint16_t pid() const { return _pid; }
    uint8_t endpoint() const { return _ep; }

    void setConnectCallback(ConnectCallback cb, void* context)
    {
        _cb = cb;
        _cbCtx = context;
    }

    void noteOn(uint8_t ch, uint8_t note, uint8_t vel);
    void noteOff(uint8_t ch, uint8_t note);
    void controlChange(uint8_t ch, uint8_t cc, uint8_t value);
    void panic();

private:
    static constexpr int QSIZE = 512;
    static constexpr int MAX_XFER = 64;

    void push(const uint8_t pkt[4]);
    bool pop(uint8_t pkt[4]);
    void submitNext();
    void openDevice(uint8_t address);
    void parseConfig(const usb_config_desc_t* cfg);
    void closeDevice();
    void fire(bool connected);
    void send2(uint8_t cin, uint8_t status, uint8_t d1, uint8_t d2);

    static void clientCb(const usb_host_client_event_msg_t* msg, void* arg);
    static void transferCb(usb_transfer_t* t);

    usb_host_client_handle_t _client = nullptr;
    usb_device_handle_t      _dev    = nullptr;
    usb_transfer_t*          _out    = nullptr;
    volatile bool            _inFlight = false;
    volatile bool            _connected = false;
    bool                     _midiFound = false;
    bool                     _seen = false;
    uint8_t                  _iface = 0;
    uint8_t                  _alt = 0;
    uint8_t                  _ep = 0;
    uint8_t                  _pkt = MAX_XFER;
    uint16_t                 _vid = 0;
    uint16_t                 _pid = 0;
    ConnectCallback          _cb = nullptr;
    void*                    _cbCtx = nullptr;

    uint8_t                  _q[QSIZE][4];
    volatile uint16_t        _head = 0;
    volatile uint16_t        _tail = 0;
};

extern UsbMidiHost usbMidi;
#endif
