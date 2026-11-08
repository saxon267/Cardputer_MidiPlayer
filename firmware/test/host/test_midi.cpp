#include <cstdio>
#include <cstring>
#include "midi.h"
#include "SD.h"

SDClass SD;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::printf("usage: test_midi file.mid\n");
        return 2;
    }

    MidiFile m;
    MidiInfo info;
    char err[64] = {0};
    if (!m.load(argv[1], info, err, sizeof(err))) {
        std::printf("FAIL: %s\n", err);
        return 1;
    }

    std::printf("dur=%u maxTick=%u tracks=%u div=%u bpm=%u.%u key=%s notes=%u events=%zu\n",
                info.durationUs, info.maxTick, info.tracks, info.division,
                info.bpmX10 / 10, info.bpmX10 % 10, info.key, info.noteCount, m.count());

    size_t n = m.count();
    size_t show = n < 32 ? n : 32;
    for (size_t i = 0; i < show; ++i) {
        const MidiEvent& e = m.events()[i];
        std::printf("%2zu t=%7u type=%u note=%3u vel=%u\n", i, e.t, e.type, e.note, e.vel);
    }
    if (n) {
        const MidiEvent& e = m.events()[n - 1];
        std::printf("last t=%u type=%u note=%u\n", e.t, e.type, e.note);
    }

    if (argc > 2 && std::strcmp(argv[2], "--dump") == 0) {
        std::printf("RESULT DUMP\n");
        return 0;
    }

    int bad = 0;
    if (info.durationUs != 5000000u) { std::printf("BAD duration\n"); ++bad; }
    if (info.tracks != 2) { std::printf("BAD tracks\n"); ++bad; }
    if (info.bpmX10 != 1200) { std::printf("BAD bpm\n"); ++bad; }
    if (m.count() != 24) { std::printf("BAD count (want 24)\n"); ++bad; }
    if (n > 0 && (m.events()[0].t != 0 || m.events()[0].type != EV_ON || m.events()[0].note != 60)) {
        std::printf("BAD first event\n"); ++bad;
    }
    if (n > 1 && m.events()[1].t != 500000u) { std::printf("BAD second event time\n"); ++bad; }
    if (n > 1 && m.events()[1].type != EV_OFF) { std::printf("BAD second event type\n"); ++bad; }
    if (n > 1 && m.events()[2].type != EV_ON) { std::printf("BAD third event type\n"); ++bad; }
    if (n && m.events()[n - 1].t != 5000000u) { std::printf("BAD last time\n"); ++bad; }

    std::printf(bad ? "RESULT FAIL\n" : "RESULT OK\n");
    return bad ? 1 : 0;
}
