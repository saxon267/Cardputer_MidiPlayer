#include <cstdio>
#include "midi.h"
#include "SD.h"

SDClass SD;

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::printf("usage: test_parse_recorded file.mid\n");
        return 2;
    }
    MidiFile m;
    MidiInfo info;
    char err[64] = {0};
    if (!m.load(argv[1], info, err, sizeof(err))) {
        std::printf("FAIL: %s\n", err);
        return 1;
    }
    std::printf("dur=%u tracks=%u div=%u bpm=%u.%u events=%zu\n",
                info.durationUs, info.tracks, info.division,
                info.bpmX10 / 10, info.bpmX10 % 10, m.count());
    if (m.count() != 8) {
        std::printf("FAIL: expected 8 events\n");
        return 1;
    }
    std::printf("RESULT OK\n");
    return 0;
}
