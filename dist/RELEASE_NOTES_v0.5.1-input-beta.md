# Cardputer MIDI Player — v0.5.1 MIDI INPUT beta

**Tag:** `v0.5.1-input-beta`  
**修复重点：** v0.5.0 录出的 `.mid` 在播放时显示 `BAD RUNNING STATUS`。

## 根因

v0.5.0 的 SMF writer 在轨道开头直接写了 tempo meta：

```text
MTrk <len> FF 51 03 ...
```

但 SMF 的每个事件（包括 meta）都必须先有一个 **delta-time VLQ**。解析器于是把 `FF` 当成 delta 的续字节，把后面的 `03` 当成事件状态，最终报 `BAD RUNNING STATUS`。

## 修复

轨道开头改为：

```text
MTrk <len> 00 FF 51 03 <tempo>
```

- 新增 1 字节 delta 0；
- track length 从 6+4 改为 7+4；
- SMF 现在可被固件自带的 `MidiFile` 解析器接受。

**注意：** v0.5.0 录出的旧 `.mid` 文件仍然是坏的，需要重新录制；v0.5.1 不能自动修复它们。

## 验证

- 录 8 个事件，生成文件 67 字节，trackLen=45；
- `MidiFile::load` 成功：
  ```text
  dur=1000000 maxTick=960 tracks=1 div=480 bpm=120.0 notes=3 events=8
  ```
- 事件时间/类型正确：
  - CH1 note on/off
  - CH2 note on/off
  - CH1 CC64 on/off
  - CH10 note on/off
- CH3 未选中，未写入。

## Host 功能

- `Tab`：PLAYER → ROLL → MIDI INPUT → PLAYER
- `1/2/3/4`：CH1 / CH2 / CH3 / CH10
- `R` / `Enter` / `Space`：开始/停止录音
- 实时钢琴卷帘、6000 事件上限
- 停止/满/离开标签页保存到：
  ```text
  /midi/MyMIDI/TAKE_0001.MID
  ```

## 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔 PC USB-C -> 断电重开 -> 接 FM-1
```

## 限制

- 只录 FM-1 自己的键盘/sequencer；外接键盘插 FM-1 TRS IN 不会被 Sloop 回传。
- Host build 无 USB-CDC；看屏幕或 `/usb.log`。
- 仍待真机复测录音/写卡/回放。
