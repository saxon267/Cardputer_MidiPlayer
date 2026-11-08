# Cardputer MIDI Player — v0.8.0 Recorder beta

**Tag:** `v0.8.0-input-beta`

## 1. 上电直接进 PLAYER

- 开机自检/SD 扫描后进入 **PLAYER**，不再先显示 FILES；
- 从 PLAYER 按 `Backspace` 进入 **FILES**；
- `Tab` 仍是四页循环：

  ```text
  PLAYER → PIANO → RECORDER → FILES → PLAYER
  ```

## 2. FILES 重命名

FILES 页选中 `.mid` / `.midi` 后按 **`N`**：

- 预填原文件名（不含扩展名）；
- 直接键入新名字（Shift 可大写）；
- `Backspace` 退格；
- `Enter` 保存；
- `Tab` 取消；
- 保留原扩展名；
- 目标名已存在时不覆盖，提示 `NAME EXISTS`。

## 3. 录音文件名去掉 TAKE

旧：

```text
LEAD_TAKE_0001.MID
```

新：

```text
LEAD_0001.MID
PAD_0001.MID
BASS_0001.MID
DRUM_0001.MID
LEAD_PAD_DRUM_0001.MID
```

## 4. 保留 v0.7 的改动

- 四页 UI；
- 按 `R` 后 ARMED，收到第一个所选 MIDI 才从 0 计时，裁掉前导静音；
- 文件在最后一个 MIDI 事件处结束；
- 方向键直接按，不需要 `Fn`；`P` / `N` 换曲；
- 要录 FM-1 sequencer，Sloop 必须 `GLO → MIDI 2 → OUT = SEQ`；
- `D` 删除 `.mid`；
- SMF tempo delta 修复。

## 5. 验证

- Host / Normal 编译通过；
- 文件名测试：
  ```text
  LEAD_0001.MID
  PAD_0001.MID
  BASS_0001.MID
  DRUM_0001.MID
  RESULT OK
  ```
- 前导 5 秒静音测试首事件仍为 `t=0`；
- SMF 解析器接受录出的文件；
- 重命名 UI 渲染通过；
- 真机重命名、SD.rename、上电页面尚未验证。

## 6. 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔 PC USB-C -> 断电重开 -> 接 FM-1
```
