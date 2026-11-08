# Cardputer MIDI Player — v0.7.0 Recorder beta

**Tag:** `v0.7.0-input-beta`

## 1. UI：四页

`Tab` 固定循环：

```text
PLAYER → PIANO → RECORDER → FILES → PLAYER
```

- **PLAYER**：播放/暂停、seek、音量、循环。
- **PIANO**：动态钢琴卷帘。
- **RECORDER**：录 FM-1 键盘/sequencer，CH1/2/3/10 选择，实时卷帘。
- **FILES**：SD 文件管理器，播放/进入目录/删除 `.mid`。

`Backspace`：

- PLAYER → FILES
- PIANO / RECORDER → PLAYER
- FILES → 上级目录

## 2. 录制从第一个 MIDI 开始

- 按 `R` 后先进入 **ARMED**，不开始计时；
- 收到第一个**所选通道**的 MIDI 事件时，`t=0`；
- 前面等待的静音不写入；
- 文件在最后一个 MIDI 事件处结束，不保留尾部静音；
- 如果一直没收到 MIDI，停止时不会生成空文件。

测试：先等 5 秒再弹第一个音，录出的第一个事件仍是 `t=0`。

## 3. 方向键不再需要 Fn

直接按：

```text
        ; = 上
, = 左      / = 右
        . = 下
```

- PLAYER / PIANO：`,` `/` 为 ±5 s；`;` `.` 为音量；
- 上一首/下一首改为 `P` / `N`；
- `Fn+[` / `Fn+]` 仍只用于普通 build 的倍速。

## 4. 录不到 sequencer 的原因

**不是 Cardputer 漏收，而是 Sloop 的 OUT 设置。**

Sloop `OUT=KEYS` 只把**键盘弹奏**从 USB 发出；`OUT=SEQ` 才会把 **sequencer、arp、rolls** 也发出。  
要录自动播放，设置：

```text
GLO → MIDI 2 → OUT = SEQ
```

键盘音符照常发出，所以 `OUT=SEQ` 时键盘和 sequencer 都能录。  
`RECORDER` 页在 ARMED 且没有事件时会显示 `WAIT MIDI... / FM1 OUT = SEQ`。

## 5. 保留

- 文件名前缀：LEAD / PAD / BASS / DRUM，多通道组合；
- `/midi/MyMIDI/<PREFIX>_TAKE_xxxx.MID`；
- FILES 页 `D` 删除 `.mid`，`Enter` 确认、`Backspace` 取消；
- v0.5.1 的 SMF tempo delta 修复；
- 6000 事件上限；Format 0 / 480 PPQ / 当前 BPM。

## 6. 验证

- Host / Normal 编译通过；
- 四通道前缀测试通过；
- 前导 5 秒静音测试：首事件 `t=0`；
- SMF 解析器接受录出的文件；
- UI 渲染通过；
- 真机录音、sequencer OUT=SEQ、删除尚未验证。
