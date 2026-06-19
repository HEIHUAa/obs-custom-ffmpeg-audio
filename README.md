# Custom FFmpeg Audio Encoder Plugin for OBS Studio

[English](#english) | [中文](#chinese)

---

## English

### Overview

Custom FFmpeg Audio is an OBS Studio plugin that provides audio encoders with **customizable FFmpeg options**. Unlike OBS's built-in FFmpeg audio encoders which have fixed parameters, this plugin allows you to pass arbitrary FFmpeg `AVDictionary` key-value options to the underlying encoder, giving you full control over the encoding process.

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

The custom options are passed directly to FFmpeg's `av_dict_parse_string()`. Write one option per line:

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

The reason is twofold:

1. **Limited OBS audio encoder property API** — OBS's `get_properties2` interface for audio encoders supports only basic property types (int, float, bool, text, list). A multi-line text editor for arbitrary FFmpeg options does not fit well into this system, and OBS provides no plugin extension point for audio encoder properties beyond what `get_properties2` offers.

2. **Independent configuration storage** — The custom options are stored in a dedicated INI file within the OBS profile directory (`obs-custom-ffmpeg-audio.ini`), separate from OBS scene collections. This ensures that:
   - Your FFmpeg tuning parameters persist across scene switches
   - They remain independent of the OBS scene data format
   - The encoder reads these options on creation, applying them before the codec is opened

For these reasons, a separate configuration window is the most practical approach for providing full control over FFmpeg options.

### License

This plugin is distributed under the GNU General Public License v2.0.

---

## Chinese

### 概述

Custom FFmpeg Audio 是一个 OBS Studio 插件，提供了一组**可自定义 FFmpeg 选项**的音频编码器。与 OBS 内置的 FFmpeg 音频编码器不同，本插件允许你将任意的 FFmpeg `AVDictionary` 键值对选项传递给底层编码器，从而完全掌控编码过程。

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

自定义选项会直接传递给 FFmpeg 的 `av_dict_parse_string()`。每行写一个选项：

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

原因有两个方面：

1. **OBS 音频编码器属性接口的限制** — OBS 的音频编码器 `get_properties2` 接口只支持有限的控件类型（整数、浮点数、布尔值、文本框、下拉列表等）。多行文本编辑器形式的任意 FFmpeg 选项不适合放在这个面板中，且 OBS 没有为音频编码器提供除 `get_properties2` 以外的插件扩展点。

2. **独立的配置存储** — 自定义选项保存在 OBS 配置目录下的独立 INI 文件中（`obs-custom-ffmpeg-audio.ini`），与 OBS 的场景集合数据分离。这样做的好处是：
   - FFmpeg 调参设置不会因切换场景而丢失
   - 配置独立于 OBS 的场景数据格式
   - 编码器在创建时从该文件读取选项，在打开 Codec 之前应用

因此，通过独立的配置窗口来提供对 FFmpeg 选项的全面控制，是最为实际可行的方案。