# Cardputer MIDI Player — 固件 v0.1.3

Cardputer-Adv 上的 SD 卡 MIDI 播放器。v0.1.3 是 **可运行的合成钢琴版**：流式 UI、SMF Format 0/1、24 声部钢琴、播放/暂停/seek/循环。

> 状态：**v0.1.3 已在实体 Cardputer-Adv 上跑通**（COM3，`sd=1` / `codec=1`，24 声部 / 2 泛音，**CPU 19–21%，`ur=0`**）。v0.1.3 修复了 note-off 包络被 attack 抵消、导致“音符永不释放/延音糊成一团”的 bug，并缩短了释音和泛音衰减。

---

## 1. 功能

| 功能 | 说明 |
|---|---|
| SD 浏览 | `/midi`，没有则回退到 `/`；文件夹 / `.mid` / `.midi`，最多 64 项 |
| SMF | Format 0 / 1，PPQ；SMPTE 和 Format 2 明确拒绝 |
| 音符 | note on/off、velocity、CC64 延音、CC7/CC11 忽略、channel 10 鼓跳过 |
| 音色 | 24 声部加法钢琴（2 个泛音 + 噪声起音），44.1 kHz 单声道 |
| 走带 | 播放 / 暂停、`Fn+,` / `Fn+/` 快退快进 5 s、上一首 / 下一首、单曲循环 |
| 音量 | 5 / 10 步进，NVS 保存 |
| UI | 240×135 Gruvbox Dark，文件列表 / 播放 / 暂停 / 结束 / 无文件 / 无卡 / 开机 |
| 状态 | 进度、总时长、BPM、调号、小节、实时音符、声部数、踏板 |

## 2. 构建与刷机

```bash
cd firmware
pio run                 # 编译
pio run -t upload       # USB-C 刷写
pio device monitor      # 115200
```

或使用合并镜像（从 `0x0` 烧写）：

```bash
pwsh tools/make_bin.ps1
# dist/CardputerMidiPlayer-8MB.bin
```

首次上机：

1. 把 `assets/test.mid` 和几首 `.mid` 放到 SD 卡的 `/midi/`；
2. 插卡、上电，串口应打印：
   ```
   [midi] audio=1 codec=1 heap=...
   [midi] sd=1 files=N heap=...
   ```
3. 列表里选文件，`Enter` 播放；`Space` 暂停/继续；`Backspace` 回列表；
4. 听到声音后看串口 `cpu=...% ur=...`：`ur` 应长期为 0，`cpu` 建议 <60%。

## 3. 按键

Cardputer 的方向键是 `Fn` 层，实际键位：

```
        Fn + ;   = 上
Fn + ,  = 左      Fn + /  = 右
        Fn + .   = 下
```

| 场景 | 键 | 动作 |
|---|---|---|
| 列表 | `Enter` / `Space` | 进目录 / 播放 |
| 列表 | `Fn+;` / `Fn+.` | 上 / 下 |
| 列表 | `Fn+,` / `Backspace` | 返回上级 |
| 列表 | `Fn+/` | 进入选中的目录 |
| 播放 | `Space` / `Enter` | 播放 / 暂停 |
| 播放 | `Fn+,` / `Fn+/` | −5 s / +5 s |
| 播放 | `Fn+;` / `Fn+.` | 音量 +5 / −5 |
| 播放 | `,` / `.` | 上一首 / 下一首 |
| 播放 | `L` | 循环开 / 关 |
| 播放 | `Backspace` | 停止并返回 |
| 全局 | `-` / `=` | 音量 −10 / +10 |

> `Fn+,` / `Fn+/` 是左右方向键（与 M5Cardputer 键位一致），因此 seek 用这两个；`Fn+;` / `Fn+.` 是上下，用于音量。

### USB 串口调试键（115200）

固件带一个很小的调试控制台，键盘不方便按的时候可以直接从串口发：

| 字符 | 动作 |
|---|---|
| `p` / 空格 | 列表里播放选中项；播放页暂停/继续 |
| `n` | 下一首 |
| `b` | 停止并回列表 |
| `+` / `-` | 音量 ±5 |
| `l` | 循环开/关 |
| `i` | 打印一行状态（screen/pos/cpu/ur/heap） |
| `h` / `?` | 帮助 |

## 4. SD 卡

```
SD:/
└── MIDI/
    ├── test.mid
    ├── CLASSICAL/
    │   └── ...
    └── POP/
        └── ...
```

- FAT32，建议 ≤32 GB；长文件名可用，但 UI 只显示 ASCII，非 ASCII 变 `?`。
- 没有 `/midi` 时自动扫根目录。
- 目录里没有 `.mid` 时显示 `NO MIDI FILES`。
- 文件名超过 26 字符会被截断；列表时长在选中时后台探测（30–100 ms），没探测的显示 `--:--`。

## 5. 实现结构

| 文件 | 职责 |
|---|---|
| `src/main.cpp` | 状态机、按键、SD/文件加载、NVS |
| `src/midi.cpp/.h` | SMF 解析、tempo map、tick→µs、事件排序 |
| `src/audio.cpp/.h` | ES8311 + I2S、24 声部钢琴、走带、seek、暂停 |
| `src/ui.cpp/.h` | 240×135 sprite、Gruvbox 组件、7 个界面 |
| `src/library.cpp/.h` | SD 挂载、目录扫描、排序、路径 |
| `src/font8.h` | Silkscreen 8 px（Regular + Bold），由 `tools/gen_font.py` 生成 |
| `test/host/` | PC 端 MIDI 解析测试（需要 g++） |

### 音频

- **不走 M5Unified Speaker**，直接 `i2s_driver_install` + ES8311 寄存器初始化；这条路在 `Cardputer_MPC` 上验证过，能控制 DMA 和块大小。
- 44.1 kHz，**256 帧/块（5.8 ms）**，6 个 DMA 缓冲（约 35 ms）；音频任务固定在 core 1、优先级 20。
- 24 声部 × 2 泛音，软件正弦表 + 线性插值；每声部增益 **0.16**，主输出 `volume² × 0.90`，有 **软限幅 + 峰值 limiter**。
- **释音 0.05 s 时间常数**（到 -63 dB 约 0.36 s），泛音衰减 **1.8 / 1.2 / 0.8 s**（低音最多 ×1.8）；修掉了“release 时 attack 还在加、包络永不下降”的 bug。
- ES8311 模拟音量固定在 **78**（不是 100），数字音量留给用户；主链路最后有 **90 Hz 高通**，减少 1 W 喇叭推不动的低频造成的破音。
- 源码用 `-O3`（`build_src_flags`，只作用于本工程源文件；M5GFX 仍用 `-Os`）。真机实测：24 声部 / 2 泛音，样本文件 **CPU 19–21%，`ur=0`**。
- 暂停用 ~50 ms 增益淡出，恢复淡入；seek 后从 0 增益淡入。
- 暂停 = 停止推进 `_posUs`，继续输出静音块；恢复从原位置继续。

### MIDI

- 两遍解析：第一遍统计事件数 / tempo map / 时长，第二遍填充事件数组。
- 事件按 tick 排序后一次性换算成 µs；播放时按 2.9 ms 块边界触发（<3 ms 抖动）。
- **v0.1.2 限制：最多 14000 个事件**（note on/off + CC64，8 B/个 = 112 KB）。超过提示 `TOO COMPLEX`。
- 同一 `(channel,note)` 的重叠 note-on 用声部叠加处理；note-off 只关闭同一通道的该 note（不会误关另一个通道的同音）。
- `CC64` 目前是全局踏板（不区分通道）；多通道钢琴文件可能踏板互串，v0.2 再做成 per-channel。
- `channel 10` 跳过；Format 2 / SMPTE 拒绝。

## 6. 内存

| 项目 | 大小 |
|---|---:|
| 静态 RAM（build 报告） | 35 KB |
| UI sprite 240×135×16bpp | 65 KB |
| 事件数组（14000×8 B） | 112 KB |
| 钢琴声部 + 正弦表 | ~7 KB |
| 剩余 heap | ~70–90 KB |

串口启动日志会打印每一步的 `heap=`；如果 `load` 报 `OUT OF MEMORY`，先把 `MAX_EVENTS` 降到 12000 或把文件裁短。

## 7. 排障

| 现象 | 检查 |
|---|---|
| 无声音 | 串口 `codec=1`？`cpu`/`ur`？耳机/喇叭音量？ |
| `sd=0` | 卡是否 FAT32、CS=12；换卡；看 `library.begin` 返回值 |
| `NO MIDI FILES` | 文件是否 `.mid`/`.midi`，是否在 `/midi` 或根目录 |
| 播放到一半断 | `ur` 是否增长；保持 `AUDIO_BLOCK=256`，避免在音频线程做重活 |
| 爆音 / 破音 | 先把音量降到 60，观察串口 `ur`/`cpu`；`ur` 增长说明欠载，`ur` 不变说明是增益/喇叭，继续降低 `es8311Init()` 音量或每声部 `gain` |
| 加载报 `TOO COMPLEX` | 文件事件数 >14000；换短一点的编曲 |
| 声音太薄/太电子 | v0.1.2 是 2 泛音合成钢琴；采样音源在 Phase 2 |
| 按键无反应 | `M5Cardputer` 是否识别 ADV；串口看 `board`/键盘初始化 |

## 8. 已知限制

- **合成音色**，不是采样钢琴；Phase 2 才上 4 MB Flash 多采样。
- **未在真机验证**；首次上机按第 2 节日志逐项确认。
- 超过 14000 事件的文件拒绝；后续可改流式解析。
- 没有按键连发、没有中文文件名渲染、没有歌词/封面。
- `CC64` 是全局踏板；多通道文件可能互串。
- 暂停/seek 是硬切（没有淡入淡出），可能有轻微咔哒；后续补 5 ms ramp。
- 44.1 kHz 输出（评估文档写 32 kHz 是设计目标；固件沿用 MPC 已验证的 44.1 kHz 路径）。

## 9. 测试

```bash
python tools/make_test_midi.py        # firmware/assets/test*.mid
# PC 端解析测试（需要 g++）：
cd firmware
g++ -std=c++17 -I test/host -I src test/host/test_midi.cpp src/midi.cpp -o test_midi
./test_midi assets/test.mid
# 期望：RESULT OK

# 合成器冒烟测试 + 3 秒试听 WAV：
g++ -std=c++17 -I src test/host/test_synth.cpp src/synth.cpp -o test_synth
./test_synth assets/demo.wav

# note-off 释音回归测试（应 ≈366 ms 后 active=0）：
g++ -std=c++17 -I src test/host/test_release.cpp src/synth.cpp -o test_release
./test_release
# 期望：RESULT OK

# UI 离屏渲染（stub M5GFX，输出 ui_*.bmp）：
g++ -std=c++17 -I test/host/ui -I test/host -I src test/host/test_ui.cpp src/ui.cpp -o test_ui
./test_ui
```

`assets/test.mid` 是 120 BPM、C 大调音阶 + C 大三和弦，4800 ticks / 5.0 s，24 个事件。
`assets/test_running.mid` / `test_tempo.mid` 覆盖 running status 与 tempo 变更。
`assets/demo.wav` 是 3 秒 C 大三和弦合成试听（44.1 kHz / 16-bit mono），可直接播放。

## 10. 许可

代码 MIT；字体 Silkscreen 为 SIL OFL 1.1（`../design/fonts/OFL.txt`）。采样音源（Phase 2）另行核对许可证。
