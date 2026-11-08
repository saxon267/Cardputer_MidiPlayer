#include <Arduino.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <string.h>

#include "app.h"
#include "audio.h"
#include "library.h"
#include "midi.h"
#include "ui.h"
#if USB_MIDI_HOST
#include "usb_midi_host.h"
#endif

App  app;
MidiFile song;

static Preferences prefs;

static void setToast(const char* msg)
{
    strncpy(app.toast, msg, sizeof(app.toast) - 1);
    app.toast[sizeof(app.toast) - 1] = 0;
    app.toastUntil = millis() + 1800;
}

static void makeTitle(const char* name, char* out, size_t n)
{
    const char* base = strrchr(name, '/');
    base = base ? base + 1 : name;
    size_t k = 0;
    for (; base[k] && base[k] != '.' && k + 1 < n && k < 22; ++k) {
        char c = base[k];
        if (c == '_') c = ' ';
        if (c >= 'a' && c <= 'z') c -= 32;
        out[k] = c;
    }
    out[k] = 0;
    while (k > 0 && out[k - 1] == ' ') out[--k] = 0;
    if (!out[0]) snprintf(out, n, "UNTITLED");
}

static void probeCurrent()
{
    if (app.sel < 0 || app.sel >= library.count()) return;
    const FileEntry& e = library.at(app.sel);
    if (e.dir || e.durSec) return;
    if (e.size > 192u * 1024u) return;  // don't block the UI on huge files

    MidiInfo tmp;
    if (song.probe(e.path, tmp)) {
        library.setDuration(app.sel, (uint16_t)(tmp.durationUs / 1000000u));
    }
}

static void adjustScroll()
{
    if (app.sel < app.scroll) app.scroll = app.sel;
    if (app.sel >= app.scroll + 6) app.scroll = app.sel - 5;
    if (app.scroll < 0) app.scroll = 0;
}

static void loadIndex(int idx)
{
    if (idx < 0 || idx >= library.count()) return;
    const FileEntry& e = library.at(idx);

    if (e.dir) {
        if (library.enter(idx)) {
            app.sel = 0;
            app.scroll = 0;
            probeCurrent();
        }
        return;
    }

    ui.loading(e.name);

    audio.stop();
    delay(30);
    song.clear();
    audio.clearSong();
    app.ev = nullptr;
    app.evCount = 0;

    char err[40] = {0};
    MidiInfo info;
    if (!song.load(e.path, info, err, sizeof(err))) {
        setToast(err[0] ? err : "LOAD FAILED");
        app.screen = Screen::LIBRARY;
        ui.render();
        return;
    }

    Serial.printf("[midi] load=%s dur=%u ev=%u heap=%u\n", e.path,
                  (unsigned)info.durationUs, (unsigned)song.count(),
                  (unsigned)ESP.getFreeHeap());
    if (song.count()) {
        Serial.printf("[midi] first t=%u last t=%u\n", (unsigned)song.events()[0].t,
                      (unsigned)song.events()[song.count() - 1].t);
    }

    app.ev = song.events();
    app.evCount = song.count();
    app.info = info;
    makeTitle(e.name, app.title, sizeof(app.title));
    app.playIndex = idx;

    audio.setSong(song.events(), song.count(), info.durationUs);
    audio.setLoop(app.loop);
    audio.setVolume(app.volume);
    audio.play();

    app.screen = Screen::PLAYER;
    ui.render();
}

static void nextTrack(int dir)
{
    int n = library.count();
    if (n <= 0) return;
    int start = app.playIndex >= 0 ? app.playIndex : app.sel;
    for (int i = 1; i <= n; ++i) {
        int idx = (start + dir * i + n * 2) % n;
        if (!library.at(idx).dir) {
            loadIndex(idx);
            return;
        }
    }
}

static void retrySd()
{
    app.sdOk = library.begin();
    if (!app.sdOk) {
        app.screen = Screen::NO_SD;
        return;
    }

    if (!library.scan("/midi")) library.scan("/");
    if (library.count() == 0) library.scan("/");

    app.sel = 0;
    app.scroll = 0;
    app.screen = library.count() ? Screen::LIBRARY : Screen::NO_FILES;
    if (app.screen == Screen::LIBRARY) probeCurrent();
}

static void backToLibrary()
{
    audio.stop();
    app.screen = Screen::LIBRARY;
    app.sel = app.playIndex >= 0 ? app.playIndex : app.sel;
    app.playIndex = -1;
    app.ev = nullptr;
    app.evCount = 0;
    adjustScroll();
}

static void printStatus()
{
    Serial.printf("[midi] dbg screen=%d sel=%d play=%d pause=%d end=%d loop=%d pos=%u/%u cpu=%u ur=%u voices=%u vol=%u spd=%.1f heap=%u\n",
                  (int)app.screen, app.sel, audio.playing() ? 1 : 0, audio.paused() ? 1 : 0,
                  audio.ended() ? 1 : 0, app.loop ? 1 : 0, (unsigned)audio.positionUs(),
                  (unsigned)audio.durationUs(), (unsigned)audio.cpuPct(),
                  (unsigned)audio.underruns(), (unsigned)audio.voices(),
                  (unsigned)app.volume, audio.speed(), (unsigned)ESP.getFreeHeap());
}

// Small USB-CDC debug console. Handy when the keyboard is not reachable.
//   p / space  play (library) or pause/resume (player)
//   n          next file
//   b          back to library
//   + / -      volume up/down
//   l          loop
//   i          status
static void changeSpeed(float delta)
{
    audio.setSpeed(audio.speed() + delta);
    prefs.putUShort("spd", (uint16_t)(audio.speed() * 1000.0f + 0.5f));
    char msg[24];
    snprintf(msg, sizeof(msg), "SPEED x%.1f", audio.speed());
    setToast(msg);
}

#if USB_MIDI_HOST
static void usbConnectCallback(bool connected, void*)
{
    app.usbConnected = connected;
    audio.setMuted(connected);
#if USB_MIDI_HOST
    if (connected) audio.resyncMidi();
#endif
}
#endif

static void handleSerial()
{
    while (Serial.available()) {
        int ci = Serial.read();
        if (ci < 0) break;
        char c = (char)ci;
        if (c == '\r' || c == '\n') continue;

        if (c == 'p' || c == 'P' || c == ' ') {
            if (app.screen == Screen::LIBRARY) {
                loadIndex(app.sel);
            } else if (app.screen == Screen::PLAYER || app.screen == Screen::ROLL) {
                if (audio.ended()) audio.play();
                else               audio.togglePause();
            }
            printStatus();
        } else if (c == 'n' || c == 'N') {
            nextTrack(+1);
        } else if (c == 'b' || c == 'B') {
            backToLibrary();
            printStatus();
        } else if (c == '+' || c == '=') {
            if (app.volume <= 95) app.volume += 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
            printStatus();
        } else if (c == '-') {
            if (app.volume >= 5) app.volume -= 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
            printStatus();
        } else if (c == 'l' || c == 'L') {
            app.loop = !app.loop;
            audio.setLoop(app.loop);
            prefs.putBool("loop", app.loop);
            printStatus();
        } else if (c == '[') {
            audio.seek(-1000000);
            printStatus();
        } else if (c == ']') {
            audio.seek(+1000000);
            printStatus();
        } else if (c == '<' || c == ',') {
            changeSpeed(-0.1f);
            printStatus();
        } else if (c == '>' || c == '.') {
            changeSpeed(+0.1f);
            printStatus();
        } else if (c >= '0' && c <= '3') {
            float target = (c == '0') ? 0.1f : (float)(c - '0');
            changeSpeed(target - audio.speed());
            printStatus();
        } else if (c == 't' || c == 'T') {
            if (app.screen == Screen::PLAYER) app.screen = Screen::ROLL;
            else if (app.screen == Screen::ROLL) app.screen = Screen::PLAYER;
            printStatus();
        } else if (c == 'i' || c == 'I') {
            printStatus();
        } else if (c == 'h' || c == 'H' || c == '?') {
            Serial.println("[midi] keys: p=play/pause n=next b=back +/-=vol l=loop [ ]=seek </>=speed t=roll i=info");
        }
    }
}

static void handleKeys()
{
    auto& ks = M5Cardputer.Keyboard.keysState();

    static bool pEnter = false, pSpace = false, pDel = false;
    static bool pUp = false, pDown = false, pLeft = false, pRight = false;
    static bool pComma = false, pDot = false, pL = false;
    static bool pMinus = false, pPlus = false;
    static bool pLBr = false, pRBr = false, pTab = false;

    bool enter = ks.enter;
    bool space = ks.space;
    bool del   = ks.del;
    bool up    = ks.fn && M5Cardputer.Keyboard.isKeyPressed(';');
    bool down  = ks.fn && M5Cardputer.Keyboard.isKeyPressed('.');
    bool left  = ks.fn && M5Cardputer.Keyboard.isKeyPressed(',');
    bool right = ks.fn && M5Cardputer.Keyboard.isKeyPressed('/');
    bool comma = !ks.fn && M5Cardputer.Keyboard.isKeyPressed(',');
    bool dot   = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('.');
    bool lkey  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('l');
    bool minus = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('-');
    bool plus  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('=');
    bool lbr   = M5Cardputer.Keyboard.isKeyPressed('[');
    bool rbr   = M5Cardputer.Keyboard.isKeyPressed(']');
    bool tab   = ks.tab;

    bool eEnter = enter && !pEnter;
    bool eSpace = space && !pSpace;
    bool eDel   = del && !pDel;
    bool eUp    = up && !pUp;
    bool eDown  = down && !pDown;
    bool eLeft  = left && !pLeft;
    bool eRight = right && !pRight;
    bool eComma = comma && !pComma;
    bool eDot   = dot && !pDot;
    bool eL     = lkey && !pL;
    bool eMinus = minus && !pMinus;
    bool ePlus  = plus && !pPlus;
    bool eLBr  = lbr && !pLBr;
    bool eRBr  = rbr && !pRBr;
    bool eTab  = tab && !pTab;

    pEnter = enter; pSpace = space; pDel = del;
    pUp = up; pDown = down; pLeft = left; pRight = right;
    pComma = comma; pDot = dot; pL = lkey;
    pMinus = minus; pPlus = plus;
    pLBr = lbr; pRBr = rbr; pTab = tab;

    // Speed: Fn + [ / ].  Plain [ / ] scrubs the song.
    if (ks.fn && eLBr) changeSpeed(-0.1f);
    else if (ks.fn && eRBr) changeSpeed(+0.1f);
    else if (app.screen == Screen::PLAYER || app.screen == Screen::ROLL) {
        if (eLBr) audio.seek(-1000000);
        if (eRBr) audio.seek(+1000000);
    }

    if (eTab && (app.screen == Screen::PLAYER || app.screen == Screen::ROLL)) {
        app.screen = (app.screen == Screen::PLAYER) ? Screen::ROLL : Screen::PLAYER;
    }

    if (eMinus && app.volume >= 10) {
        app.volume -= 10;
        audio.setVolume(app.volume);
        prefs.putUChar("vol", app.volume);
    }
    if (ePlus && app.volume <= 90) {
        app.volume += 10;
        audio.setVolume(app.volume);
        prefs.putUChar("vol", app.volume);
    }

    if (app.screen == Screen::LIBRARY) {
        int n = library.count();
        if (eUp && n) { app.sel = (app.sel + n - 1) % n; adjustScroll(); probeCurrent(); }
        if (eDown && n) { app.sel = (app.sel + 1) % n; adjustScroll(); probeCurrent(); }
        if (eEnter || eSpace) loadIndex(app.sel);
        if (eLeft || eDel) {
            if (library.up()) {
                app.sel = 0;
                app.scroll = 0;
                probeCurrent();
            }
        }
        if (eRight) loadIndex(app.sel);
    } else if (app.screen == Screen::PLAYER) {
        if (eEnter || eSpace) {
            if (audio.ended()) audio.play();
            else               audio.togglePause();
        }
        if (eDel) backToLibrary();
        if (eLeft)  audio.seek(-5000000);
        if (eRight) audio.seek(+5000000);
        if (eUp && app.volume <= 95) {
            app.volume += 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
        }
        if (eDown && app.volume >= 5) {
            app.volume -= 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
        }
        if (eComma) nextTrack(-1);
        if (eDot)   nextTrack(+1);
        if (eL) {
            app.loop = !app.loop;
            audio.setLoop(app.loop);
            prefs.putBool("loop", app.loop);
        }
    } else if (app.screen == Screen::ROLL) {
        if (eEnter || eSpace) {
            if (audio.ended()) audio.play();
            else               audio.togglePause();
        }
        if (eDel) app.screen = Screen::PLAYER;
        if (eLeft)  audio.seek(-5000000);
        if (eRight) audio.seek(+5000000);
        if (eUp && app.volume <= 95) {
            app.volume += 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
        }
        if (eDown && app.volume >= 5) {
            app.volume -= 5;
            audio.setVolume(app.volume);
            prefs.putUChar("vol", app.volume);
        }
        if (eComma) nextTrack(-1);
        if (eDot)   nextTrack(+1);
        if (eL) {
            app.loop = !app.loop;
            audio.setLoop(app.loop);
            prefs.putBool("loop", app.loop);
        }
    } else if (app.screen == Screen::NO_SD) {
        if (eEnter || eDel) retrySd();
    } else if (app.screen == Screen::NO_FILES) {
        if (eEnter || eDel) retrySd();
    }
}

void setup()
{
    Serial.begin(115200);
    delay(60);
    Serial.printf("\n[midi] boot v%s, heap=%u\n", APP_VERSION, (unsigned)ESP.getFreeHeap());

    auto cfg = M5.config();
    cfg.internal_spk = false;
    cfg.internal_mic = false;
    cfg.internal_imu = false;
    cfg.internal_rtc = false;
    M5Cardputer.begin(cfg, true);
    M5Cardputer.Display.setBrightness(140);

    prefs.begin("midi", false);
    app.volume = prefs.getUChar("vol", 78);
    if (app.volume > 100) app.volume = 78;
    app.loop = prefs.getBool("loop", false);
    audio.setSpeed(prefs.getUShort("spd", 1000) / 1000.0f);

    ui.begin();
    Serial.printf("[midi] heap after ui=%u\n", (unsigned)ESP.getFreeHeap());
    ui.boot(0, 0);
    delay(100);

    bool audioOk = audio.begin();
    Serial.printf("[midi] audio=%d codec=%d heap=%u\n", audioOk ? 1 : 0, audio.codecOk() ? 1 : 0,
                  (unsigned)ESP.getFreeHeap());
    audio.setVolume(app.volume);
    audio.setLoop(app.loop);
    if (audioOk) audio.start();

#if USB_MIDI_HOST
    usbMidi.setConnectCallback(usbConnectCallback, nullptr);
    app.usbHostOk = usbMidi.begin();
    if (!app.usbHostOk) setToast("USB HOST FAIL");
#endif

    app.sdOk = library.begin();
    if (app.sdOk) {
        if (!library.scan("/midi")) library.scan("/");
        if (library.count() == 0) library.scan("/");
    }
    Serial.printf("[midi] sd=%d files=%d heap=%u\n", app.sdOk ? 1 : 0, library.count(),
                  (unsigned)ESP.getFreeHeap());
    for (int i = 0; i < library.count() && i < 12; ++i) {
        const FileEntry& e = library.at(i);
        Serial.printf("[midi] file %d %c %s %u bytes\n", i, e.dir ? 'D' : 'F', e.name,
                      (unsigned)e.size);
    }

    ui.boot(55, library.count());
    delay(180);
    ui.boot(100, library.count());
    delay(240);

    app.battery = M5.Power.getBatteryLevel();
    if (app.battery > 100 || app.battery < 0) app.battery = 80;

    if (!app.sdOk) {
        app.screen = Screen::NO_SD;
    } else if (library.count() == 0) {
        app.screen = Screen::NO_FILES;
    } else {
        app.screen = Screen::LIBRARY;
        app.sel = 0;
        app.scroll = 0;
        probeCurrent();
    }

    ui.render();
    if (!audioOk) {
        setToast("AUDIO INIT FAILED");
        ui.render();
    } else if (!audio.codecOk()) {
        setToast("NO CODEC");
        ui.render();
    }

    Serial.printf("[midi] ready: sd=%d files=%d heap=%u\n", app.sdOk ? 1 : 0,
                  library.count(), (unsigned)ESP.getFreeHeap());
    Serial.println("[midi] serial keys: p=play/pause n=next b=back +/-=vol l=loop i=info");
}

void loop()
{
    M5Cardputer.update();
#if USB_MIDI_HOST
    usbMidi.poll();
    static bool lastUsb = false;
    if (app.usbConnected != lastUsb) {
        lastUsb = app.usbConnected;
        setToast(app.usbConnected ? "FM1 CONNECTED" : "FM1 DISCONNECTED");
    }
#endif
    handleSerial();
    handleKeys();

    static uint32_t lastRender = 0;
    static uint32_t lastBat    = 0;
    uint32_t now = millis();

    if (now - lastBat > 10000) {
        lastBat = now;
        int b = M5.Power.getBatteryLevel();
        if (b >= 0 && b <= 100) app.battery = (uint8_t)b;
    }

    if (now - lastRender >= 100) {
        lastRender = now;
        ui.render();
    }

    static uint32_t lastDiag = 0;
    if (now - lastDiag >= 5000) {
        lastDiag = now;
        Serial.printf("[midi] pos=%u/%u cpu=%u%% ur=%u heap=%u\n",
                      (unsigned)audio.positionUs(), (unsigned)audio.durationUs(),
                      (unsigned)audio.cpuPct(), (unsigned)audio.underruns(),
                      (unsigned)ESP.getFreeHeap());
    }

    delay(2);
}
