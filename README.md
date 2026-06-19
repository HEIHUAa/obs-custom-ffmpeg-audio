# Custom FFmpeg Audio Encoder Plugin for OBS Studio

[English](#english) | [中文](#chinese)

---

## English

### Overview

Custom FFmpeg Audio is an OBS Studio plugin that provides audio encoders with **customizable FFmpeg options**. Unlike OBS's built-in FFmpeg audio encoders which have fixed parameters, this plugin allows you to set any FFmpeg encoder options (such as `aac_coder=twoloop`, `application=audio`, etc.) to fine-tune the encoding behavior.

### Supported Audio Formats

| Encoder Family | Sub-encoders / Codecs |
|---|---|
| AAC | AAC (FFmpeg `aac`) |
| Opus | Opus (`libopus`) |
| FLAC | FLAC 16-bit, FLAC 24-bit |
| ALAC | ALAC 16-bit, ALAC 24-bit |
| MP3 | MP3 (`libmp3lame`) |
| AC3 | AC3, E-AC3 (`eac3`) |
| PCM 16-bit | PCM signed 16-bit little-endian (`pcm_s16le`) |
| PCM 24-bit | PCM signed 24-bit little-endian (`pcm_s24le`) |
| PCM 32-bit | PCM signed 32-bit little-endian (`pcm_s32le`) |
| PCM 32-bit float | PCM 32-bit float little-endian (`pcm_f32le`) |
| PCM 8-bit | PCM unsigned 8-bit (`pcm_u8`) |

### How to Use

1. Go to **Settings → Advanced → Audio** (or use Simple output mode's audio settings) and select the desired "Custom FFmpeg Audio (...)" encoder for your audio track.
2. In the OBS main window, click the **Tools** menu on the top menu bar, then select **Custom FFmpeg Audio Settings**.
3. In the configuration dialog, you can:
   - Select the encoder family (AAC, Opus, FLAC, etc.)
   - Choose a specific sub-encoder/codec
   - Set sample rate
   - Toggle quality-based mode (for supported encoders)
   - **Write custom FFmpeg options** in the text area (see format below)
4. Click **Save** — the settings take effect when you restart recording/streaming.

### Custom Options Format

Write one option per line, using `key=value` format:

```
option1=value1
option2=value2
```

Examples:

```
# For AAC (sets AAC encoder to use VBR):
aac_coder=twoloop

# For libopus (set complexity and application type):
complexity=10
application=audio

# For libmp3lame (set encoding quality):
quality=2
```

Unrecognized or unused options will be logged as warnings and ignored — they will not cause the encoder to fail.

### Why Not Put Custom Options in the Encoder Properties?

You might wonder: why are the custom options configured through a separate **Tools** menu window instead of being integrated into the encoder's properties panel (visible in Advanced Output settings)?

The reason is simple:

1. **OBS audio encoder settings area is not customizable** — In OBS's Advanced Output mode, the audio encoder settings panel does not allow plugins to add any custom controls or make any modifications. Unlike video encoders, OBS does not provide a plugin extension point for audio encoder property panels. Therefore, any additional configuration must be done outside of this area.

For this reason, a separate configuration window is the most practical approach for providing full control over FFmpeg options.

### License

This plugin is distributed under the GNU General Public License v2.0.

---

## Chinese

### 概述

Custom FFmpeg Audio 是一个 OBS Studio 插件，提供了一组**可自定义 FFmpeg 选项**的音频编码器。与 OBS 内置的 FFmpeg 音频编码器不同，本插件允许你自由设置 FFmpeg 编码器参数（比如 `aac_coder=twoloop`、`application=audio` 等），从而灵活调整编码行为。

### 支持的音频格式

| 编码器系列 | 子编码器 / Codec |
|---|---|
| AAC | AAC (FFmpeg `aac`) |
| Opus | Opus (`libopus`) |
| FLAC | FLAC 16-bit、FLAC 24-bit |
| ALAC | ALAC 16-bit、ALAC 24-bit |
| MP3 | MP3 (`libmp3lame`) |
| AC3 | AC3、E-AC3 (`eac3`) |
| PCM 16-bit | PCM 有符号 16-bit 小端 (`pcm_s16le`) |
| PCM 24-bit | PCM 有符号 24-bit 小端 (`pcm_s24le`) |
| PCM 32-bit | PCM 有符号 32-bit 小端 (`pcm_s32le`) |
| PCM 32-bit float | PCM 32-bit 浮点小端 (`pcm_f32le`) |
| PCM 8-bit | PCM 无符号 8-bit (`pcm_u8`) |

### 使用方法

1. 进入 **设置 → 高级 → 音频**（或在简单输出模式下选择音频编码器），为你的音轨选择对应的 "Custom FFmpeg Audio (...)" 编码器。
2. 在 OBS 主窗口顶部的菜单栏中，点击 **工具(Tools)** 菜单，选择 **Custom FFmpeg Audio Settings**。
3. 在弹出的配置对话框中，你可以：
   - 选择编码器系列（AAC、Opus、FLAC 等）
   - 选择具体的子编码器/Codec
   - 设置采样率
   - 启用基于质量的编码模式（仅部分编码器支持）
   - **在文本框中编写自定义 FFmpeg 选项**（格式见下文）
4. 点击 **保存(Save)** — 设置会在下次开始录制/推流时生效。

### 自定义选项格式

每行写一个选项，格式为 `key=value`：

```
option1=value1
option2=value2
```

例如：

```
# AAC 使用双环编码器（VBR 模式）
aac_coder=twoloop

# libopus 设置复杂度与编码用途
complexity=10
application=audio

# libmp3lame 设置编码质量
quality=2
```

无法识别或未使用的选项会被记录为警告日志并被忽略 — 不会导致编码器初始化失败。

### 为何不把自定义选项放在编码器属性面板中？

你可能会问：为什么自定义选项要通过独立的**工具**菜单窗口配置，而不是直接集成到编码器的属性面板（即在高级输出模式中看到的设置界面）中？

原因很简单：

1. **OBS 音频编码器设置区域无法自定义** — 在 OBS 的高级输出模式中，音频编码器的设置面板不允许插件添加任何控件选项，也无法做任何修改。与视频编码器不同，OBS 没有为音频编码器提供属性面板的插件扩展点，所以额外配置必须在面板之外实现。

因此，通过独立的配置窗口来提供对 FFmpeg 选项的全面控制，是最为实际可行的方案。