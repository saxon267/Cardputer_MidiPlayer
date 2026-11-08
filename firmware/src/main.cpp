#include <Arduino.h>
#include <M5Cardputer.h>
#include <Preferences.h>
#include <SD.h>
#include <string.h>

#include "app.h"
#include "audio.h"
#include "library.h"
#include "midi.h"
#include "ui.h"
#if USB_MIDI_HOST
#include "usb_midi_host.h"
#include "midi_recorder.h"
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
    app.playIndex = -1;
    app.screen = Screen::PLAYER;
    app.info = MidiInfo{};
    app.title[0] = 0;
    app.ev = nullptr;
    app.evCount = 0;
    if (library.count()) probeCurrent();
}

static void refreshLibrary()
{
    char path[96];
    strncpy(path, library.path(), sizeof(path) - 1);
    path[sizeof(path) - 1] = 0;
    library.scan(path);
    int n = library.count();
    if (app.sel >= n) app.sel = n ? n - 1 : 0;
    if (app.sel < 0) app.sel = 0;
    adjustScroll();
    probeCurrent();
}

static void backToLibrary()
{
    audio.stop();
    refreshLibrary();
    app.screen = Screen::LIBRARY;
    app.sel = app.playIndex >= 0 ? app.playIndex : app.sel;
    app.playIndex = -1;
    app.ev = nullptr;
    app.evCount = 0;
    if (app.sel >= library.count()) app.sel = library.count() ? library.count() - 1 : 0;
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
static void saveTake()
{
    char path[64] = {0};
    if (rec.save(path, sizeof(path))) setToast(path);
    else                               setToast("SAVE FAILED");
}

static void stopTake()
{
    if (rec.recording() || rec.fullTake()) {
        rec.stopRecording();
        saveTake();
    }
}
#endif

static void deleteSelected()
{
    app.confirmDelete = false;
    if (!app.sdOk || app.sel < 0 || app.sel >= library.count()) {
        setToast("NO FILE");
        return;
    }
    const FileEntry& e = library.at(app.sel);
    if (e.dir) {
        setToast("NOT A FILE");
        return;
    }
    char path[96];
    strncpy(path, e.path, sizeof(path) - 1);
    path[sizeof(path) - 1] = 0;
    if (!SD.remove(path)) {
        setToast("DELETE FAIL");
        return;
    }
    app.playIndex = -1;
    char current[96];
    strncpy(current, library.path(), sizeof(current) - 1);
    current[sizeof(current) - 1] = 0;
    library.scan(current);
    int n = library.count();
    if (app.sel >= n) app.sel = n ? n - 1 : 0;
    if (app.sel < 0) app.sel = 0;
    adjustScroll();
    probeCurrent();
    setToast("DELETED");
}

static void startRename()
{
    if (!app.sdOk || app.sel < 0 || app.sel >= library.count()) {
        setToast("NO FILE");
        return;
    }
    const FileEntry& e = library.at(app.sel);
    if (e.dir) {
        setToast("NOT A FILE");
        return;
    }
    const char* dot = strrchr(e.name, '.');
    size_t stem = dot ? (size_t)(dot - e.name) : strlen(e.name);
    if (stem > sizeof(app.renameBuf) - 1) stem = sizeof(app.renameBuf) - 1;
    memcpy(app.renameBuf, e.name, stem);
    app.renameBuf[stem] = 0;
    if (dot && strlen(dot) < sizeof(app.renameExt)) {
        strcpy(app.renameExt, dot);
    } else {
        strcpy(app.renameExt, ".MID");
    }
    app.renaming = true;
    app.confirmDelete = false;
}

static void cancelRename()
{
    app.renaming = false;
    setToast("CANCELED");
}

static void finishRename()
{
    if (!app.renaming) return;
    app.renaming = false;
    if (!app.sdOk || app.sel < 0 || app.sel >= library.count()) {
        setToast("NO FILE");
        return;
    }
    if (!app.renameBuf[0]) {
        setToast("EMPTY NAME");
        return;
    }

    char newName[48];
    snprintf(newName, sizeof(newName), "%s%s", app.renameBuf, app.renameExt);

    const FileEntry& e = library.at(app.sel);
    char oldPath[96];
    strncpy(oldPath, e.path, sizeof(oldPath) - 1);
    oldPath[sizeof(oldPath) - 1] = 0;

    char dir[96];
    const char* slash = strrchr(oldPath, '/');
    if (slash) {
        size_t n = (size_t)(slash - oldPath);
        if (n >= sizeof(dir)) n = sizeof(dir) - 1;
        memcpy(dir, oldPath, n);
        dir[n] = 0;
    } else {
        dir[0] = 0;
    }

    char target[160];
    if (dir[0]) snprintf(target, sizeof(target), "%s/%s", dir, newName);
    else        snprintf(target, sizeof(target), "/%s", newName);

    if (strcasecmp(oldPath, target) == 0) {
        setToast("UNCHANGED");
        return;
    }
    if (SD.exists(target)) {
        setToast("NAME EXISTS");
        return;
    }
    if (!SD.rename(oldPath, target)) {
        setToast("RENAME FAIL");
        return;
    }

    char cur[96];
    strncpy(cur, library.path(), sizeof(cur) - 1);
    cur[sizeof(cur) - 1] = 0;
    library.scan(cur);
    for (int i = 0; i < library.count(); ++i) {
        if (strcmp(library.at(i).name, newName) == 0) {
            app.sel = i;
            break;
        }
    }
    adjustScroll();
    probeCurrent();
    setToast("RENAMED");
}

#if USB_MIDI_HOST
static void usbLog(const char* msg)
{
    if (!app.sdOk) return;
    File f = SD.open("/usb.log", FILE_APPEND);
    if (!f) return;
    f.printf("[%lu] %s\n", (unsigned long)millis(), msg);
    f.close();
}

static void usbConnectCallback(bool connected, void*)
{
    app.usbConnected = connected;
    audio.setMuted(connected);
#if USB_MIDI_HOST
    if (connected) {
        audio.requestPanic();
        audio.requestResync();
    }
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
    static bool pL = false, pP = false, pN = false;
    static bool pMinus = false, pPlus = false;
    static bool pLBr = false, pRBr = false, pTab = false;
    static bool pR = false, p1 = false, p2 = false, p3 = false, p4 = false, p0 = false;
    static bool pD = false;

    bool enter = ks.enter;
    bool space = ks.space;
    bool del   = ks.del;
    bool up    = M5Cardputer.Keyboard.isKeyPressed(';');
    bool down  = M5Cardputer.Keyboard.isKeyPressed('.');
    bool left  = M5Cardputer.Keyboard.isKeyPressed(',');
    bool right = M5Cardputer.Keyboard.isKeyPressed('/');
    bool prevKey = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('p');
    bool nextKey = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('n');
    bool lkey  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('l');
    bool minus = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('-');
    bool plus  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('=');
    bool lbr   = M5Cardputer.Keyboard.isKeyPressed('[');
    bool rbr   = M5Cardputer.Keyboard.isKeyPressed(']');
    bool tab   = ks.tab;
    bool rkey  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('r');
    bool dkey  = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('d');
    bool d1    = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('1');
    bool d2    = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('2');
    bool d3    = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('3');
    bool d4    = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('4');
    bool d0    = !ks.fn && M5Cardputer.Keyboard.isKeyPressed('0');

    bool eEnter = enter && !pEnter;
    bool eSpace = space && !pSpace;
    bool eDel   = del && !pDel;
    bool eUp    = up && !pUp;
    bool eDown  = down && !pDown;
    bool eLeft  = left && !pLeft;
    bool eRight = right && !pRight;
    bool ePrev  = prevKey && !pP;
    bool eNext  = nextKey && !pN;
    bool eL     = lkey && !pL;
    bool eMinus = minus && !pMinus;
    bool ePlus  = plus && !pPlus;
    bool eLBr  = lbr && !pLBr;
    bool eRBr  = rbr && !pRBr;
    bool eTab  = tab && !pTab;
    bool eR    = rkey && !pR;
    bool eD    = dkey && !pD;
    bool e1    = d1 && !p1;
    bool e2    = d2 && !p2;
    bool e3    = d3 && !p3;
    bool e4    = d4 && !p4;
    bool e0    = d0 && !p0;

    pEnter = enter; pSpace = space; pDel = del;
    pUp = up; pDown = down; pLeft = left; pRight = right;
    pL = lkey; pP = prevKey; pN = nextKey;
    pMinus = minus; pPlus = plus;
    pLBr = lbr; pRBr = rbr; pTab = tab;
    pR = rkey; p1 = d1; p2 = d2; p3 = d3; p4 = d4; p0 = d0;
    pD = dkey;

    if (app.renaming) {
        for (char c : ks.word) {
            if (c < 32 || c > 126) continue;
            if (strchr("/\\:*?\"<>|", c)) continue;
            size_t n = strlen(app.renameBuf);
            if (n < sizeof(app.renameBuf) - 1) {
                app.renameBuf[n] = c;
                app.renameBuf[n + 1] = 0;
            }
        }
        if (eDel) {
            size_t n = strlen(app.renameBuf);
            if (n) app.renameBuf[n - 1] = 0;
        }
        if (eEnter) finishRename();
        else if (eTab) cancelRename();
        return;
    }

    if (app.confirmDelete) {
        if (eEnter || eSpace) {
            deleteSelected();
        } else if (eDel || eD) {
            app.confirmDelete = false;
            setToast("CANCELED");
        }
        return;
    }

    // Speed: Fn + [ / ].  Plain [ / ] scrubs the song.
#if !USB_MIDI_HOST
    if (ks.fn && eLBr) changeSpeed(-0.1f);
    else if (ks.fn && eRBr) changeSpeed(+0.1f);
    else
#endif
    if ((app.screen == Screen::PLAYER || app.screen == Screen::ROLL) && !ks.fn) {
        if (eLBr) audio.seek(-1000000);
        if (eRBr) audio.seek(+1000000);
    }

    if (eTab) {
        if (app.screen == Screen::LIBRARY) {
            app.screen = Screen::PLAYER;
        } else if (app.screen == Screen::PLAYER) {
            app.screen = Screen::ROLL;
        } else if (app.screen == Screen::ROLL) {
            app.screen = Screen::MIDI_IN;
#if USB_MIDI_HOST
            if (!rec.recording()) rec.beginPreview();
#endif
        } else if (app.screen == Screen::MIDI_IN) {
#if USB_MIDI_HOST
            stopTake();
#endif
            refreshLibrary();
            app.screen = Screen::LIBRARY;
        }
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
        if (eD) {
            if (n && !library.at(app.sel).dir) app.confirmDelete = true;
            else                              setToast("SELECT MIDI FILE");
        }
        if (eNext) {
            if (n && !library.at(app.sel).dir) startRename();
            else                              setToast("SELECT MIDI FILE");
        }
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
        if (ePrev) nextTrack(-1);
        if (eNext) nextTrack(+1);
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
        if (ePrev) nextTrack(-1);
        if (eNext) nextTrack(+1);
        if (eL) {
            app.loop = !app.loop;
            audio.setLoop(app.loop);
            prefs.putBool("loop", app.loop);
        }
    } else if (app.screen == Screen::MIDI_IN) {
#if USB_MIDI_HOST
        if (eDel) {
            stopTake();
            app.screen = Screen::PLAYER;
        }
        if (eR || eEnter || eSpace) {
            if (rec.recording()) {
                stopTake();
            } else if (!app.usbConnected) {
                setToast("CONNECT FM1");
            } else if (!usbMidi.inputEndpoint()) {
                setToast("NO MIDI IN");
            } else {
                rec.startRecording(app.bpmX10);
            }
        }
        if (!rec.recording()) {
            if (e1) rec.toggleChannel(0);
            if (e2) rec.toggleChannel(1);
            if (e3) rec.toggleChannel(2);
            if (e4 || e0) rec.toggleChannel(9);
        }
#else
        if (eDel) app.screen = Screen::PLAYER;
#endif
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
    usbMidi.setInputCallback([](uint8_t s, uint8_t d1, uint8_t d2, void*) {
        rec.add(s, d1, d2);
    }, nullptr);
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
    } else {
        app.screen = Screen::PLAYER;
        app.sel = 0;
        app.scroll = 0;
        app.playIndex = -1;
        app.info = MidiInfo{};
        app.title[0] = 0;
        app.ev = nullptr;
        app.evCount = 0;
        if (library.count()) probeCurrent();
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
    uint32_t now = millis();
#if USB_MIDI_HOST
    app.usbDeviceSeen = usbMidi.deviceSeen();
    static bool lastUsb = false;
    static bool lastSeen = false;
    static bool hostLogged = false;
    if (!hostLogged) {
        hostLogged = true;
        char b[80];
        snprintf(b, sizeof(b), "host=%s", app.usbHostOk ? "ok" : "FAIL");
        usbLog(b);
    }
    if (app.usbDeviceSeen != lastSeen) {
        lastSeen = app.usbDeviceSeen;
        char b[80];
        snprintf(b, sizeof(b), "device seen=%d midi=%d vid=%04x pid=%04x ep=%02x in=%02x",
                 app.usbDeviceSeen ? 1 : 0, usbMidi.midiFound() ? 1 : 0,
                 (unsigned)usbMidi.vid(), (unsigned)usbMidi.pid(),
                 (unsigned)usbMidi.endpoint(), (unsigned)usbMidi.inputEndpoint());
        usbLog(b);
    }
    if (app.usbConnected != lastUsb) {
        lastUsb = app.usbConnected;
        usbLog(app.usbConnected ? "FM1 connected" : "FM1 disconnected");
        setToast(app.usbConnected ? "FM1 CONNECTED" : "FM1 DISCONNECTED");
    }

    // Follow FM-1's USB MIDI clock as the master tempo. With no clock, play
    // the file at its own BPM (speed 1.0).
    uint32_t srcX10 = app.info.bpmX10;
    if (srcX10 < 100) srcX10 = 1200;
    float fmBpm = usbMidi.bpm();
    bool fromFm1 = app.usbConnected && fmBpm > 0.0f;
    uint32_t targetX10 = fromFm1 ? (uint32_t)(fmBpm * 10.0f + 0.5f) : srcX10;
    if (targetX10 < 100) targetX10 = srcX10;
    audio.setSpeed((float)targetX10 / (float)srcX10);
    app.bpmX10 = (uint16_t)targetX10;
    app.bpmFromFm1 = fromFm1;

    static bool lastFromFm1 = false;
    if (fromFm1 != lastFromFm1) {
        lastFromFm1 = fromFm1;
        setToast(fromFm1 ? "FM1 BPM LOCK" : "BPM FREE");
    }

    static uint32_t lastBpmLog = 0;
    if (app.usbConnected && now - lastBpmLog > 5000) {
        lastBpmLog = now;
        char b[96];
        snprintf(b, sizeof(b), "bpm=%s%u in=%02x clock=%d",
                 fromFm1 ? "F" : "B", (unsigned)(app.bpmX10 / 10),
                 (unsigned)usbMidi.inputEndpoint(), usbMidi.clockValid() ? 1 : 0);
        usbLog(b);
    }

    static bool lastPaused = false;
    if (audio.paused() != lastPaused) {
        lastPaused = audio.paused();
        char b[80];
        snprintf(b, sizeof(b), "pause=%d panicNotes=%u active=%u",
                 lastPaused ? 1 : 0, (unsigned)usbMidi.lastPanicNotes(),
                 (unsigned)usbMidi.activeNotes());
        usbLog(b);
    }

    static bool fullSaved = false;
    if (rec.fullTake()) {
        if (!fullSaved) {
            fullSaved = true;
            rec.stopRecording();
            saveTake();
        }
    } else {
        fullSaved = false;
    }
#endif
    handleSerial();
    handleKeys();

    static uint32_t lastRender = 0;
    static uint32_t lastBat    = 0;

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
