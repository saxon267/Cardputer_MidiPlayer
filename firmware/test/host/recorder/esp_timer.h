#pragma once
#include <stdint.h>

extern int64_t g_test_us;
inline int64_t esp_timer_get_time() { return g_test_us; }
