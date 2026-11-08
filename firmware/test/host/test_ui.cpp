#include <cstdio>
#include <cstring>

#define private public
#include "app.h"
#include "audio.h"
#include "library.h"
#undef private

#include "ui.h"

App app;
Library library;
AudioEngine audio;
unsigned long millis() { return 0; }

static void setLib()
{
    library._n = 6;
    strcpy(library._path, "/midi");
    const char* names[6] = {"CHOPIN", "DEBUSSY", "CLAIR_DE_LUNE.MID", "FLOWER_DANCE.MID",
                            "GYMNOPEDIE_1.MID", "RIVER_FLOWS_IN_YOU.MID"};
    uint32_t sizes[6] = {0, 0, 18432, 14336, 11264, 21504};
    uint16_t durs[6] = {0, 0, 227, 151, 192, 246};
    bool dirs[6] = {true, true, false, false, false, false};
    for (int i = 0; i < 6; ++i) {
        FileEntry& e = library._e[i];
        memset(&e, 0, sizeof(e));
        strncpy(e.name, names[i], sizeof(e.name) - 1);
        if (strcmp(library._path, "/") == 0)
            snprintf(e.path, sizeof(e.path), "/%s", names[i]);
        else
            snprintf(e.path, sizeof(e.path), "%s/%s", library._path, names[i]);
        e.size = sizes[i];
        e.durSec = durs[i];
        e.dir = dirs[i];
    }
}

static MidiEvent rollEv[] = {
    {70000000, EV_ON, 60, 100, 0}, {72600000, EV_OFF, 60, 0, 0},
    {70200000, EV_ON, 64, 96, 0},  {72100000, EV_OFF, 64, 0, 0},
    {70400000, EV_ON, 67, 104, 0}, {73100000, EV_OFF, 67, 0, 0},
    {70600000, EV_ON, 72, 90, 0},  {72400000, EV_OFF, 72, 0, 0},
    {71400000, EV_ON, 76, 110, 1}, {73400000, EV_OFF, 76, 0, 1},
    {71600000, EV_ON, 79, 88, 1},  {72900000, EV_OFF, 79, 0, 1},
    {71800000, EV_ON, 81, 100, 2}, {73900000, EV_OFF, 81, 0, 2},
    {72200000, EV_ON, 84, 80, 2},  {74400000, EV_OFF, 84, 0, 2},
};

static void setPlaying()
{
    app.screen = Screen::PLAYER;
    app.volume = 78;
    app.loop = false;
    app.battery = 82;
    app.sdOk = true;
    strcpy(app.title, "CLAIR DE LUNE");
    app.info.tracks = 9;
    app.info.division = 480;
    app.info.maxTick = 4800;
    app.info.tsNum = 4;
    app.info.tsDen = 4;
    app.info.bpmX10 = 660;
    strcpy(app.info.key, "DB MAJ");
    app.info.durationUs = 227000000;
    app.ev = rollEv;
    app.evCount = sizeof(rollEv) / sizeof(rollEv[0]);

    audio._durationUs = 227000000;
    audio._posUs = 72000000;
    audio._paused = false;
    audio._ended = false;
    audio._sustainOn = true;
    audio._voices = 3;
    audio._speedX1000 = 1000;
    audio._active[0] = 0;
    audio._active[1] = (1u << (61 - 32));
    audio._active[2] = (1u << (65 - 64)) | (1u << (68 - 64));
    audio._active[3] = 0;
}

static void save(const char* path)
{
    g_ui_out = path;
    ui.render();
}

int main()
{
    ui.begin();
    setLib();
    app.sdOk = true;
    app.battery = 82;
    app.volume = 78;

    // library
    app.screen = Screen::LIBRARY;
    app.sel = 2;
    app.scroll = 0;
    app.playIndex = 2;
    app.info = MidiInfo{};
    save("ui_library.bmp");

    app.confirmDelete = true;
    save("ui_delete.bmp");
    app.confirmDelete = false;

    // playing
    setPlaying();
    save("ui_player.bmp");

    // piano roll
    setPlaying();
    app.screen = Screen::ROLL;
    app.loop = true;
    audio._posUs = 72000000;
    audio._voices = 6;
    save("ui_roll.bmp");

    // paused
    setPlaying();
    audio._paused = true;
    save("ui_paused.bmp");

    // ended
    setPlaying();
    audio._ended = true;
    audio._paused = false;
    audio._posUs = audio._durationUs;
    audio._voices = 0;
    audio._active[0] = audio._active[1] = audio._active[2] = audio._active[3] = 0;
    save("ui_ended.bmp");

    // no files
    app.screen = Screen::NO_FILES;
    app.sdOk = true;
    library._n = 0;
    strcpy(library._path, "/midi");
    save("ui_nofiles.bmp");

    // no sd
    app.screen = Screen::NO_SD;
    app.sdOk = false;
    save("ui_nosd.bmp");

    // boot
    g_ui_out = "ui_boot.bmp";
    ui.boot(55, 12);

    g_ui_out = "ui_loading.bmp";
    ui.loading("CLAIR_DE_LUNE.MID");

    std::printf("wrote ui_*.bmp\n");
    return 0;
}
