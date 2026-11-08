# Cardputer MIDI Player — v0.6.0 MIDI INPUT beta

**Tag:** `v0.6.0-input-beta`  
在 v0.5.1 的 MIDI INPUT 基础上增加通道前缀文件名和文件删除。

## 1. 录音文件名前缀

停止录音后写入 `/midi/MyMIDI/`，文件名按所选通道生成：

| 通道 | 前缀 | 示例 |
|---|---|---|
| CH1 | `LEAD` | `LEAD_TAKE_0001.MID` |
| CH2 | `PAD` | `PAD_TAKE_0001.MID` |
| CH3 | `BASS` | `BASS_TAKE_0001.MID` |
| CH10 | `DRUM` | `DRUM_TAKE_0001.MID` |
| 多通道 | 按固定顺序连接 | `LEAD_PAD_DRUM_TAKE_0001.MID` |

- 每个前缀各自递增编号，不会覆盖其他通道的 take。
- 只录选中的通道；未选通道不写入文件。

## 2. 文件删除

在 **LIBRARY** 列表：

- `D`：对选中的 `.mid` / `.midi` 打开删除确认；
- `Enter` / `Space`：确认删除；
- `Backspace` / `D`：取消；
- 目录不能删除；
- 删除后重新扫描当前目录，列表和选中项自动收敛。

## 3. 保留功能

- `Tab`：PLAYER → ROLL → MIDI INPUT → PLAYER
- `1/2/3/4`：CH1 / CH2 / CH3 / CH10
- `R` / `Enter` / `Space`：录音开始/停止
- 实时钢琴卷帘；6000 事件上限
- SMF Format 0、480 PPQ；当前 BPM tempo；Note On/Off + CC64
- v0.5.1 的 tempo delta 修复已包含

## 4. 验证

- Host / Normal 编译通过；
- SMF 单元测试：`LEAD_PAD_DRUM_TAKE_0001.MID`，8 事件，CH1/2/10 正确，CH3 过滤，CC64=2；
- 固件 `MidiFile::load` 解析录出的文件成功；
- 删除流程尚未做 SD 真机测试。

## 5. 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔 PC USB-C -> 断电重开 -> 接 FM-1
```
