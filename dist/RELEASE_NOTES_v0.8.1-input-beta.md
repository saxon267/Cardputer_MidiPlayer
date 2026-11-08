# Cardputer MIDI Player — v0.8.1 Recorder beta

**Tag:** `v0.8.1-input-beta`  
修复 v0.8.0 的两个严重问题。

## 1. `.mid` 播放失败 / 回到文件列表

- v0.5.0 录出的旧文件缺少 tempo meta 前的 delta 0，解析器报 `BAD RUNNING STATUS`；
- v0.8.1 的解析器对“轨道开头就是 `FF 51 03`”的旧文件做兼容：补一个 delta 0 后继续解析；
- 正常文件格式不变，v0.5.1 之后录的文件仍按标准解析；
- 加载失败现在会：
  - 屏幕 toast 显示具体错误；
  - 串口打印 `load fail path=... err=...`；
  - Host build 追加到 `/usb.log` 的 `loadfail ...`。

如果唯一的 `.mid` 是 v0.5.0 时代的旧 take，v0.8.1 应能直接播放；如果仍失败，`/usb.log` 会给出具体 parser 错误。

## 2. 重命名输入一个字母出现一大串

根因：`Keyboard.keysState().word` 每帧都包含**当前仍按住的字符**，旧代码每帧都 append，于是按住一帧就重复很多次。

v0.8.1：

- 进入重命名时先记录当前键盘字符作为基线，不把打开重命名的 `N` 写进去；
- 之后按“当前字符 - 上一帧字符”的差集追加；
- 长按不会重复，只有新按下的键才加入；
- `Backspace` 退格，`Enter` 保存，`Tab` 取消，逻辑不变。

## 3. 保留 v0.8.0

- 上电直接进 PLAYER；
- 四页：PLAYER / PIANO / RECORDER / FILES；
- FILES `N` 重命名、`D` 删除；
- 录音文件名 `LEAD_0001.MID` 等，无 TAKE；
- ARMED 后第一个 MIDI 才起录，裁掉前导静音；
- 方向键直接按。

## 4. 验证

- Host / Normal 编译通过；
- 构造 v0.5.0 形状的旧文件（去掉 delta 0、trackLen-1），`MidiFile::load` 现在成功：
  ```text
  dur=1000000 tracks=1 div=480 bpm=120.0 events=8
  RESULT OK
  ```
- 标准 v0.8 文件仍解析成功；
- 重命名 UI 渲染通过。

## 5. 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔 PC USB-C -> 断电重开 -> 接 FM-1
```
