# Cardputer MIDI Player — v0.5.0 MIDI INPUT beta

**Tag:** none yet (test build)  
**Host-only feature** · normal player unchanged.

## 新增

- `Tab` 在 USB-C Host build 循环：

  ```text
  PLAYER → PIANO ROLL → MIDI INPUT → PLAYER
  ```

- **MIDI INPUT** 页录 FM-1 自己的键盘/sequencer（经 USB MIDI OUT 回传）。
- 通道多选：`1` / `2` / `3` / `4` =
  **CH1 / CH2 / CH3 / CH10**；至少保留一个，默认 CH1。
- 实时钢琴卷帘，只显示所选通道；CH1 橙、CH2 绿、CH3 黄、CH10 蓝。
- `R` / `Enter` / `Space` 开始/停止录音。
- 停止后写入：

  ```text
  /midi/MyMIDI/TAKE_0001.MID
  /midi/MyMIDI/TAKE_0002.MID
  ```

- SMF Format 0，480 PPQ，单轨；当前 BPM 的 tempo；Note On/Off + CC64。
- 最多 6000 个事件，满了自动停止并保存。

## 重要限制

- 只录 **FM-1 自己的键盘/sequencer**；外接 MIDI 键盘插 FM-1 的 TRS MIDI IN **不会**被 Sloop 回传到 USB。
- 外接键盘要录，需要直接接 Cardputer 的 TRS/USB 输入（USB-C 已被 FM-1 占用时用 TRS/Unit MIDI）。
- `cardputer_midi_usb` 没有 USB-CDC 串口；看屏幕或 `/usb.log`。
- 本版尚未真机复测 MIDI INPUT / SMF 写卡；SMF writer 已通过主机单元测试。
- Host 功能仍为 experimental；普通播放器、卷帘、倍速和串口版为稳定部分。

## 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 先拔 PC USB-C，断电重开，再接 FM-1
```

## 文件

- `CardputerMidiPlayer-USBHost-8MB.bin`：含 MIDI INPUT 的 Host build，0x0。
- `CardputerMidiPlayer-8MB.bin`：普通 build，0x0。
- `CHEATSHEET.md`：按键/Sloop/排障。
- `SHA256SUMS.txt`。
