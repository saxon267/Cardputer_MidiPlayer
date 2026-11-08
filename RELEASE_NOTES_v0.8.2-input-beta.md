# Cardputer MIDI Player — v0.8.2 Recorder beta

**Tag:** `v0.8.2-input-beta`  
针对“读取某个 `.mid` 时固件重启，随后 PLAYER 显示 UNTITLED”。

## 判断

这不是解析失败后回到文件列表，而是**加载过程中设备复位**：

- 启动读条重新出现；
- 播放器没有曲目；
- 说明 `loadIndex()` 里的解析过程触发了 watchdog、栈溢出或内存破坏。
- 文件越大、事件越多，越容易触发。

## 修复

1. **解析大文件时喂 watchdog**
   - `parseFile()` 每读 16 KB 调用一次 `delay(1)`；
   - 主循环不再长时间独占 CPU，避免 task watchdog 复位；
   - 解析仍是阻塞的，但不会饿死 idle task。

2. **事件上限在计数阶段就生效**
   - 原实现只在 fill 阶段检查 `MAX_EVENTS`，countOnly 可以先数出远大于 14000 的事件，再申请大块内存；
   - 现在 countOnly 到达 14000 就标记 overflow，直接返回 `TOO COMPLEX`，不会走到大内存申请。

3. **Ctx 从栈移到静态存储**
   - `Ctx` 含 256 个 tempo，约 2 KB；`load()` 同时有 c1/c2，外加 Reader 512 B，主循环栈余量很紧；
   - `probe`/`load` 的 Ctx 改为静态并显式 reset，降低栈溢出风险；
   - BSS 增加约 6 KB。

4. **加载失败可追踪**
   - 屏幕 toast 显示 parser 错误；
   - 串口 `load fail path=... err=...`；
   - Host build 写入 `/usb.log` 的 `loadfail ...`。

5. **旧 v0.5.0 文件兼容**
   - 轨道开头直接是 `FF 51 03` 的旧 take 按 delta 0 解析；
   - 标准文件不变。

## 验证

- Host / Normal 编译通过；
- 旧格式文件：
  ```text
  dur=1000000 tracks=1 div=480 bpm=120.0 events=8
  RESULT OK
  ```
- 标准 v0.8 文件：
  ```text
  dur=1000000 tracks=1 div=480 bpm=120.0 events=8
  RESULT OK
  ```
- 超过 14000 事件的文件现在应返回 `TOO COMPLEX`，而不是进入大内存申请。

## 仍需要你测

- 触发重启的那个具体 `.mid`；
- 如果仍重启，请记录 `/usb.log` 最后一行，或把该文件发我；
- 如果返回 `TOO COMPLEX`，说明文件事件数超过 14000，需要缩短/拆分。

## 烧写

```bash
pwsh tools/flash_usbhost.ps1 -Port COMx
# 拔 PC USB-C -> 断电重开 -> 接 FM-1
```
