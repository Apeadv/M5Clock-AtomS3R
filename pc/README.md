# M5Clock PC spectrum sender

这个小工具把电脑正在播放的声音转成 12 段频谱，通过 USB 串口发给 AtomS3R。

不需要拆机，不需要麦克风，也不需要蓝牙音频。

## 最快用法：双击 vbs

Windows 直接双击：

```text
M5Clock-Spectrum.vbs
```

它会在后台静默运行，不出现黑色命令行窗口，也不占任务栏，不会注册关闭快捷键。

第一次运行会弹出一个对话框，询问是否设置开机自启动。选“否”不会添加任何启动项。

之后会自动安装依赖、找 AtomS3R 的串口并开始发送。

需要 Python 3.9 或更高版本。脚本会优先用 `py`，没有就用 `python`。

需要看日志或排查问题时，才用 `M5Clock-Spectrum.bat`，它会显示命令行窗口。

## 开机自启动

第一次双击 `M5Clock-Spectrum.vbs` 时会问一次，选“是”才添加。

也可以之后手动运行：

```text
Install-Autostart.bat
```

会在 Windows 启动文件夹里创建一个静默快捷方式（wscript + vbs）。以后开机自动运行，不出现黑框，设备没插也会一直等待，插上 AtomS3R 后自动开始发送。

取消自启动：

```text
Remove-Autostart.bat
```

## 手动运行

```bash
py -m pip install -r pc/requirements.txt
py pc/m5clock_spectrum.py
```

不写 `--port` 会自动检测 AtomS3R。检测不到时用：

```bash
py pc/m5clock_spectrum.py --list
py pc/m5clock_spectrum.py --port COM4
```

## 手动指定串口 / 扬声器

先关闭 Arduino Serial Monitor、M5Burner 的串口窗口，以及其他占用串口的软件。

```bash
py pc/m5clock_spectrum.py --list
```

会列出串口和扬声器。AtomS3R 通常显示为 `COMx`。

如果自动检测不对，或者默认扬声器不是正在放歌的那个：

```bash
py pc/m5clock_spectrum.py --port COM4 --device "扬声器名称"
```

## 4. 在设备上设置

配网页里这是两个独立菜单：

```text
AtomS3R 屏幕 -> 音乐频谱
频谱音频来源 -> USB 电脑
Chain Mono 显示 -> 自定义字幕
```

这样 S3R 显示频谱，三个 Chain Mono 继续显示字幕，两者可以同时工作。

电脑端脚本开始发送后，S3R 会显示 12 段彩色频谱。

## 常见问题

### 打开串口失败

- 关闭 Arduino Serial Monitor / M5Burner 串口窗口。
- 确认 `--port` 是 AtomS3R 的串口，不是蓝牙串口。
- Windows 设备管理器里看一下有没有黄色感叹号。

### 换了音频设备后没有频谱

脚本默认会自动跟随 Windows 的默认播放设备。切换耳机 / 音箱后，等 1–2 秒会重新打开 loopback。

如果用了固定的 `--device`，它不会自动跟随；去掉 `--device` 或改成新设备名。

### 没有声音 / 频谱不动

- 确认电脑正在播放声音。
- 确认 `--device` 选的是当前输出设备。
- Windows 如果没有 loopback，可以在声音设置里启用“立体声混音”，或者换一个支持 loopback 的输出设备。
- 先运行 `--list`，确认扬声器列表。

### 蓝牙可以吗

不行。AtomS3R 的 ESP32-S3 只有 BLE，没有 Classic Bluetooth / A2DP，不能直接接收蓝牙音频。

目前支持的是 **USB 电脑连接**。如果以后要做蓝牙，需要额外的蓝牙音频接收硬件，或者在手机/电脑端做 BLE 音频转发，复杂度会高很多。

### 会不会影响烧录

不会。这个脚本只通过 USB 串口发送频谱数据，不修改固件。烧录时先关掉脚本即可。

## 协议

如果以后想自己写发送端，协议很简单：

```text
0xA5 0x5A | 12 个频段值（0-8） | checksum
checksum = 0x5A XOR 所有频段值
```

每帧 15 字节，默认 30fps。
