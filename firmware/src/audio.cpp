#include "audio.h"

#include <Arduino.h>
#include <M5Cardputer.h>
#include <driver/i2s.h>
#include <math.h>
#include <string.h>
#include "synth.h"

AudioEngine audio;


// ---------------------------------------------------------------- ES8311
static bool es8311Write(uint8_t reg, uint8_t val)
{
    return M5Cardputer.In_I2C.writeRegister8(0x18, reg, val, 400000);
}

static uint8_t dacReg(uint8_t volume)
{
    if (volume == 0) return 0x00;
    if (volume <= 50) return (uint8_t)(0x60 + (volume * (0xBF - 0x60)) / 50);
    return (uint8_t)(0xC0 + ((volume - 51) * (0xFF - 0xC0)) / 49);
}

static void es8311Init(uint8_t volume)
{
    es8311Write(0x00, 0x80);
    es8311Write(0x01, 0xB5);
    es8311Write(0x02, 0x18);
    es8311Write(0x09, 0x0C);
    es8311Write(0x0A, 0x0C);
    es8311Write(0x0D, 0x01);
    es8311Write(0x12, 0x00);
    es8311Write(0x13, 0x10);
    es8311Write(0x32, dacReg(volume));
    es8311Write(0x37, 0x08);
    delay(30);
}

static bool installI2s()
{
    i2s_config_t cfg = {};
    cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    cfg.sample_rate          = AUDIO_RATE;
    cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_16BIT;
    cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
    cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
    cfg.dma_buf_count        = 6;
    cfg.dma_buf_len          = AUDIO_BLOCK;
    cfg.use_apll             = false;
    cfg.tx_desc_auto_clear   = true;
    cfg.fixed_mclk           = 0;
    cfg.mclk_multiple        = I2S_MCLK_MULTIPLE_256;
    cfg.bits_per_chan        = I2S_BITS_PER_CHAN_16BIT;

    if (i2s_driver_install(I2S_NUM_0, &cfg, 0, nullptr) != ESP_OK) return false;

    i2s_pin_config_t pins = {};
    pins.mck_io_num   = I2S_PIN_NO_CHANGE;
    pins.bck_io_num   = PIN_I2S_BCLK;
    pins.ws_io_num    = PIN_I2S_LRCK;
    pins.data_out_num = PIN_I2S_DOUT;
    pins.data_in_num  = I2S_PIN_NO_CHANGE;
    return i2s_set_pin(I2S_NUM_0, &pins) == ESP_OK;
}

// ---------------------------------------------------------------- engine
static Piano* gPiano = nullptr;

bool AudioEngine::begin()
{
    gPiano = new Piano();
    if (!gPiano) return false;
    gPiano->init();

    _codecOk = M5Cardputer.In_I2C.scanID(0x18, 400000);
    if (_codecOk) es8311Init(100);

    i2s_driver_uninstall(I2S_NUM_0);
    if (!installI2s()) return false;
    return true;
}

void AudioEngine::start()
{
    xTaskCreatePinnedToCore(taskEntry, "midi-audio", 6144, this, 20, (TaskHandle_t*)&_task, 1);
}

void AudioEngine::taskEntry(void* arg)
{
    static_cast<AudioEngine*>(arg)->taskLoop();
}

void AudioEngine::apply(const MidiEvent& e)
{
    switch (e.type) {
        case EV_ON:      gPiano->noteOn(e.note, e.vel, e.pad); break;
        case EV_OFF:     gPiano->noteOff(e.note, e.pad);       break;
        case EV_SUS_ON:  gPiano->setSustain(true);  _sustainOn = true;  break;
        case EV_SUS_OFF: gPiano->setSustain(false); _sustainOn = false; break;
        case EV_ALL_OFF: gPiano->allOff();          _sustainOn = false; break;
        default: break;
    }
}

void AudioEngine::resetPlayback()
{
    _idx    = 0;
    _posUs  = 0;
    _frames = 0;
    _ended  = false;
    _sustainOn = false;
    _voices = 0;
    _active[0] = _active[1] = _active[2] = _active[3] = 0;
    if (gPiano) gPiano->reset();
}

void AudioEngine::handleSeek()
{
    uint32_t target = _seekTo;
    if (target > _durationUs) target = _durationUs;
    _posUs = target;
    _frames = (uint64_t)target * AUDIO_RATE / 1000000ull;
    _ended = false;
    _idx   = 0;
    if (gPiano) gPiano->reset();

    size_t lo = 0, hi = _count;
    while (lo < hi) {
        size_t mid = (lo + hi) / 2;
        if (_events[mid].t < target) lo = mid + 1;
        else                         hi = mid;
    }
    _idx = lo;
}

void AudioEngine::taskLoop()
{
    static int16_t mono[AUDIO_BLOCK];
    static int16_t out[AUDIO_BLOCK * 2];

    uint32_t cpuAcc = 0;
    int      cpuN   = 0;

    while (true) {
        if (_seekReq) {
            handleSeek();
            _seekReq = false;
        }

        uint32_t t0 = micros();
        bool active = _playing && !_paused;

        if (active && gPiano) {
            uint32_t blockUs = (uint32_t)((uint64_t)AUDIO_BLOCK * 1000000ull / AUDIO_RATE);
            uint32_t endUs   = _posUs + blockUs;

            while (_idx < _count && _events[_idx].t <= endUs) {
                apply(_events[_idx]);
                ++_idx;
            }

            float vol = _volume / 100.0f;
            gPiano->render(mono, AUDIO_BLOCK, vol * vol * 1.2f);
            gPiano->fillMask(_active);
            _voices = gPiano->activeCount();

            _frames += AUDIO_BLOCK;
            _posUs = (uint32_t)(_frames * 1000000ull / AUDIO_RATE);

            if (_idx >= _count && _posUs >= _durationUs + 400000u) {
                if (_loop) {
                    resetPlayback();
                } else {
                    _ended   = true;
                    _playing = false;
                    gPiano->allOff();
                    _voices = 0;
                    _active[0] = _active[1] = _active[2] = _active[3] = 0;
                    _sustainOn = false;
                }
            }
        } else {
            memset(mono, 0, sizeof(mono));
        }

        for (int i = 0; i < AUDIO_BLOCK; ++i) {
            out[i * 2]     = mono[i];
            out[i * 2 + 1] = mono[i];
        }

        size_t written = 0;
        esp_err_t err = i2s_write(I2S_NUM_0, out, sizeof(out), &written, portMAX_DELAY);
        if (err != ESP_OK) {
            ++_underruns;
            vTaskDelay(1);
        }

        uint32_t spent = micros() - t0;
        cpuAcc += spent;
        if (++cpuN >= 16) {
            uint32_t blockUs = (uint32_t)((uint64_t)AUDIO_BLOCK * 1000000ull / AUDIO_RATE);
            uint32_t pct = cpuAcc * 100u / (blockUs * (uint32_t)cpuN);
            _cpu = (uint8_t)(pct > 100 ? 100 : pct);
            cpuAcc = 0;
            cpuN   = 0;
        }
    }
}

void AudioEngine::setSong(const MidiEvent* events, size_t count, uint32_t durationUs)
{
    _playing  = false;
    _paused   = false;
    _events   = events;
    _count    = count;
    _durationUs = durationUs;
    resetPlayback();
}

void AudioEngine::clearSong()
{
    _playing = false;
    _paused  = false;
    _events  = nullptr;
    _count   = 0;
    _durationUs = 0;
    resetPlayback();
}

void AudioEngine::play()
{
    if (!_events || _count == 0) return;
    if (_ended) resetPlayback();
    _playing = true;
    _paused  = false;
}

void AudioEngine::togglePause()
{
    if (!_playing || _ended) return;
    _paused = !_paused;
}

void AudioEngine::stop()
{
    _playing = false;
    _paused  = false;
    _ended   = false;
    resetPlayback();
}

void AudioEngine::seek(int32_t deltaUs)
{
    if (!_events || _count == 0) return;
    int64_t target = (int64_t)_posUs + deltaUs;
    if (target < 0) target = 0;
    if (target > _durationUs) target = _durationUs;
    if (_ended) {
        _ended   = false;
        _playing = true;
    }
    _seekTo  = (uint32_t)target;
    _seekReq = true;
}

void AudioEngine::setLoop(bool on) { _loop = on; }

void AudioEngine::setVolume(uint8_t v)
{
    _volume = v > 100 ? 100 : v;
}
