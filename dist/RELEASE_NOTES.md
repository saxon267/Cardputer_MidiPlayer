# Cardputer MIDI Player — v0.4.0 Release Notes

**Release date:** 2026-11-09  
**Tag:** `v0.4.0`

Cardputer-Adv 上的 SD 卡 MIDI 播放器。普通播放器为稳定功能；USB-C Host 直连 M-VAVE FM-1 / Sloop 仍标记为 **experimental**。

## 下载

| 文件 | 烧写地址 | 用途 |
|---|---|---|
| `dist/CardputerMidiPlayer-8MB.bin` | `0x0` | 普通版：USB-CDC 串口、本地合成器、钢琴卷帘、倍速 |
| `dist/CardputerMidiPlayer-USBHost-8MB.bin` | `0x0` | USB-C Host：直连 FM-1，无 USB-CDC，屏幕 + `/usb.log` |
| `CHEATSHEET.md` | — | 一页速查表（按键、Sloop、排障） |
| `dist/SHA256SUMS.txt` | — | 镜像校验 |

## 功能

- SD `/midi`（回退 `/`）、文件夹、`.mid` / `.midi`，最多 64 项
- SMF Format 0 / 1，tempo map、running status、CC64；SMPTE / Format 2 拒绝
- 24 声部 / 2 泛音合成钢琴，44.1 kHz，ES8311 + I2S
- 播放 / 暂停 / seek / 循环 / 上一首 / 下一首
- `[` / `]` ±1 s，`Fn+,` / `Fn+/` ±5 s
- 普通版 `Fn+[` / `Fn+]` 0.1–3.0 倍速
- 钢琴卷帘：`Tab` 切换，8 秒窗口，纵向自动跟随音域
- Gruvbox Dark 240×135 UI，8 个界面
- 普通版 USB-CDC 串口控制台
- **实验**：USB-C Host 发送 Note On/Off、CC64、All Notes Off；channel 10 鼓转发；连接静音本机喇叭；FM-1 MIDI clock BPM 跟随；暂停显式 Note Off；`/usb.log` 诊断

## Host 已知问题

- **暂停挂音**：先检查 Sloop `ARP 2 → HOLD=OFF`。Sloop 的 KBD HOLD 会锁住 MIDI 音符，Cardputer 无法用标准 Note Off 绕过。
- **BPM 跟随**：需要 Sloop `GLO → MIDI 3 → CLOCK=OUT` 且 `SYNC=INT`；显示 `F120` 才成功，`B66` 表示没收到 clock。
- Host build **没有 USB-CDC**；调试用 `/usb.log` 或屏幕。
- 最新暂停竞态修复 / BPM 跟随仍需完整真机复测；发布说明不宣称已修复所有 Sloop 侧行为。
- FM-1 是 USB MIDI + Audio 复合设备；Host 只 claim MIDIStreaming，忽略 Audio。

## 烧写

普通版：

```bash
pio run -e cardputer_midi -t upload
# 或 esptool write_flash 0x0 dist/CardputerMidiPlayer-8MB.bin
```

Host 版（务必用 `no_reset` 脚本，刷完先拔 PC 线）：

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔掉 PC USB-C -> 断电重开 -> 接 FM-1
```

恢复普通串口固件：

```bash
# 断开 FM-1，按住 G0 进下载模式
pwsh tools/flash_normal.ps1 -Port COMx
```

## Sloop 设置（Host）

```text
GLO → MIDI       CH1=1  CH2=2  CH3=3  CH4=10
GLO → MIDI 2     IN=NOTES   OTHER=SEL/OFF
GLO → MIDI 3     CLOCK=OUT
SYNC=INT
DR MAP=GM
ARP 2 → HOLD=OFF
```

## 限制

- 最多 14000 个事件（约 7000 个音符），超过 `TOO COMPLEX`
- 合成音色，不是采样钢琴；Phase 2 未实现
- 无按键连发；中文文件名显示为 `?`
- Host 功能仍为实验性

## 许可

MIT，见 [LICENSE](LICENSE)。
