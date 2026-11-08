# M5Clock v1.4.0 — 音乐频谱、自定义时区与双语界面

发布日期：2026-11-07  
适用硬件：AtomS3R（8MB Flash）+ Atomic toChain Base + 3× Chain Mono（U217）

---

## 中文

### 版本亮点

M5Clock v1.4.0 在 v1.3.2 的基础上，新增了 **音乐频谱**。三个 Chain Mono 始终显示时钟，AtomS3R 单独切换世界时间 / 音乐频谱，自定义字幕整合在频谱底部。

- **音乐频谱**
  - AtomS3R 屏幕单独选择“世界时间 / 音乐频谱”
  - 三个 Chain Mono 始终显示时钟
  - AtomS3R 128×128 显示 12 段彩色频谱
  - 彩色渐变柱、落峰、倒影、低频边框脉冲和 LIVE 指示
  - 自定义字幕整合在频谱底部，21 字符内居中，更长滚动
  - 默认 USB 电脑输入，运行 `pc/m5clock_spectrum.py`，不需要额外硬件
  - 可选 PDM 麦克风（M5Stack Unit Mini PDM），接 Grove Port A
  - 蓝牙音频不支持：ESP32-S3 只有 BLE，没有 Classic Bluetooth / A2DP
  - 没有数据时 S3R 显示 `NO PC` / `NO MIC`
- **频谱字幕**
  - 配网页“频谱字幕”最多 32 个 ASCII 字符
  - 直接显示在音乐频谱底部
  - 21 个字符以内居中；更长自动横向滚动
  - 三个 Chain Mono 始终显示时钟
  - 点阵屏垂直位置可选“上方”或“下方（整体下移 1 行）”，只作用于时钟
  - 使用 5×7 点阵字库，支持大写/小写字母、数字和常用符号
- **点阵屏时区**
  - 配网页可选择北京、伦敦、纽约、洛杉矶、东京、悉尼、UTC
  - 支持自定义 UTC+X：x 为用户输入的 -12 到 +14 整数小时
  - 固定城市自动处理夏令时；自定义偏移不使用夏令时
  - S3R 城市列表中，已选时区排第一，左侧小条为黄色；其他城市为蓝色
  - 选择 UTC 或自定义 UTC+X 时，北京默认排第一，小条为蓝色
- **滚屏速度**
  - 配网页可选择慢 / 正常 / 快
  - 在 AtomS3R 上双击按键也可以循环切换速度
- **短按切换模式**
  - AtomS3R 屏幕**不支持触摸**
  - 使用屏幕下方的按键短按，在“时钟”和“自定义字符”之间切换
  - 世界时间页仍然每 7 秒自动轮播，不受影响
- **中英双语**
  - 配网页顶部切换 中文 / English
  - AtomS3R 屏幕同步切换中文/英文界面
- **世界时间**
  - UTC 大字时间、日期、星期
  - 北京、伦敦、纽约、洛杉矶、东京、悉尼轮播
  - 自动处理夏令时
- **配网与交互**
  - 首次上电 AP 配网
  - NTP 自动对时
  - 可调 Mono 顺序、Mono 亮度、屏幕亮度
  - 长按按键 3 秒恢复配网

### 相对 v1.3.2 的变更

- 新增音乐频谱模式
- 新增 USB 电脑频谱输入（默认）和可选 PDM 麦克风输入
- 新增 256 点 FFT
- 新增 12 段彩色频谱显示：S3R 彩色柱状，字幕整合在底部
- 短按按键切换 Chain Mono 的时钟 / 字幕 / 频谱
- 无电脑数据时 S3R 显示 `NO PC`；PDM 模式无信号时显示 `NO MIC`
- 音乐频谱不依赖 Wi-Fi 或 NTP

### 烧录

文件：

```text
M5Clock-AtomS3R-M5Burner.bin
```

步骤：

1. 打开 M5Burner，进入 **Custom Firmware / 自定义固件**
2. 选择 `M5Clock-AtomS3R-M5Burner.bin`
3. 烧录地址：`0x0`
4. Flash Size：`8MB`
5. AtomS3R 按住按键直到绿色 LED 亮起，进入下载模式
6. 点击 Burn

SHA-256：

```text
1c96cd076d992ba10919d1a7f211417a7a837dd8935fc85fc39d931b764f94ba
```

### 升级注意

- 整片 8MB 镜像可能覆盖 NVS，升级后建议重新配网。
- 频谱字幕只支持 ASCII；中文暂不支持 5×7 点阵显示。
- 字幕超过 21 个字符会自动滚动，而不是静态缩小。
- 短按按键现在切换 Mono 模式；世界时间页不需要手动翻页。

### 已知说明

- 英文界面城市名使用 8px 字体，属于 128×128 屏幕的紧凑布局。
- 配网页首次加载会扫描附近 Wi-Fi，可能需要 1–3 秒。
- 只支持 2.4GHz Wi-Fi。

### 致谢

- M5Stack / lovyan03：M5GFX
- M5Stack：M5Chain
- Adafruit GFX：5×7 ASCII 字库（BSD-3-Clause）
- Noto Sans SC Bold（SIL OFL 1.1）：中文字库来源

---

## English

### Highlights

M5Clock v1.4.0 adds **music spectrum** on top of v1.3.2. The three Chain Mono modules always show the clock; the AtomS3R switches between world clock and spectrum, with the subtitle integrated at the bottom of the spectrum.

- **Music spectrum**
  - The AtomS3R screen switches between world clock and music spectrum
  - The three Chain Mono modules always show the clock
  - 12-band color spectrum on the AtomS3R 128×128 LCD
  - Gradient bars, falling peaks, reflection, bass pulse, and LIVE indicator
  - The subtitle is integrated at the bottom of the spectrum
  - USB PC input by default; run `pc/m5clock_spectrum.py`, no extra hardware
  - Optional PDM microphone (M5Stack Unit Mini PDM) on Grove Port A
  - Bluetooth audio is not supported: ESP32-S3 is BLE-only, without Classic Bluetooth / A2DP
  - Shows `NO PC` / `NO MIC` when no data is available
- **Custom dot-matrix text**
  - New “Dot-matrix mode” section in the setup page
  - Choose “Clock” or “Custom text”
  - Up to 32 ASCII characters
  - Up to 4 characters are centered; longer text scrolls horizontally
  - Vertical position: top, or bottom (shifted down one row); applies to both clock and custom text
  - 5×7 bitmap font with upper/lowercase letters, digits, and common symbols
- **Dot-matrix time zone**
  - Select Beijing, London, New York, Los Angeles, Tokyo, Sydney, or UTC
  - Custom UTC+X: enter an integer offset from -12 to +14 hours
  - Fixed cities use automatic DST; custom offsets do not
  - On the S3R city list, the selected zone is first with a yellow bar; other cities use blue
  - With UTC or custom UTC+X, Beijing stays first with a blue bar
- **Scroll speed**
  - Slow / normal / fast in the setup page
  - Double-click the AtomS3R button to cycle speed on-device
- **Short-press mode switch**
  - The AtomS3R screen is **not a touchscreen**
  - Short-press the button below the screen to cycle clock / custom text / music spectrum
  - The world-clock page still rotates automatically every 7 seconds
- **Bilingual UI**
  - Switch 中文 / English on the setup page
  - The AtomS3R screen switches language as well
- **World clock**
  - Large UTC clock, date, and weekday
  - Rotating Beijing, London, New York, Los Angeles, Tokyo, and Sydney
  - Automatic DST
- **Setup and controls**
  - First-boot AP portal
  - NTP synchronization
  - Configurable Mono order, Mono brightness, and LCD brightness
  - Hold the button for 3 seconds to re-enter setup

### Changes since v1.3.2

- Added music spectrum mode
- Added USB PC spectrum input (default) and optional PDM microphone input
- Added a 256-point FFT
- Added a 12-band color spectrum on the S3R, with the subtitle integrated at the bottom
- Short press now switches the S3R between world clock and music spectrum
- Shows `NO PC` without PC data; `NO MIC` in PDM mode without a signal
- Music spectrum does not require Wi-Fi or NTP

### Flash

File:

```text
M5Clock-AtomS3R-M5Burner.bin
```

1. Open M5Burner → **Custom Firmware**
2. Select `M5Clock-AtomS3R-M5Burner.bin`
3. Flash address: `0x0`
4. Flash size: `8MB`
5. Hold the AtomS3R button until the green LED turns on
6. Click **Burn**

SHA-256:

```text
1c96cd076d992ba10919d1a7f211417a7a837dd8935fc85fc39d931b764f94ba
```

### Upgrade notes

- The full 8MB image may overwrite NVS; reconfigure Wi-Fi after upgrading.
- Custom text supports ASCII only. Chinese characters are not supported on the dot-matrix display.
- Text longer than 4 characters scrolls instead of shrinking.
- Short press now switches the Chain Mono display between clock / subtitle / spectrum.
- Music spectrum uses USB PC by default (`pc/m5clock_spectrum.py`); PDM microphone is optional.

### Known limitations

- English city names use an 8px font to fit the 128×128 display.
- The first setup page load scans nearby Wi-Fi networks and may take 1–3 seconds.
- 2.4GHz Wi-Fi only.
- Bluetooth audio is not supported (ESP32-S3 is BLE-only); use USB PC or an optional PDM microphone.

### Credits

- M5Stack / lovyan03 — M5GFX
- M5Stack — M5Chain
- Adafruit GFX — 5×7 ASCII font (BSD-3-Clause)
- Noto Sans SC Bold (SIL OFL 1.1) — Chinese bitmap font source


















