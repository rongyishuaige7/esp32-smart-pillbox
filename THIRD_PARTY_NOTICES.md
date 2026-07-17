# 第三方声明

本仓库中的 Rongyi 原创 ESP32 固件、Flutter 客户端和整理文档以根目录 [MIT License](LICENSE) 公开。构建依赖由 PlatformIO、Flutter、Gradle 或包管理器按需下载；本仓库不复制 PlatformIO 缓存、Flutter SDK、ESP32 框架、第三方库源码、真实 Wi-Fi 配置、APK、构建二进制或实物/EDA 材料。

## 固件构建与库

| 组件 | 当前公开工程中的版本/约束 | 用途 | 许可来源 |
| :-- | :-- | :-- | :-- |
| PlatformIO Core | `6.1.19` | 固件构建与包管理 | [Apache-2.0](https://github.com/platformio/platformio-core/blob/develop/LICENSE) |
| Espressif32 Platform | `espressif32@6.13.0`（由公开 `platformio.ini` 固定） | ESP32 平台与工具定义 | [Apache-2.0](https://github.com/platformio/platform-espressif32/blob/develop/LICENSE) |
| Arduino-ESP32 framework | PlatformIO 在本机构建中解析为 `3.20017.241212+sha.dcc1105b` | Wi-Fi、WebServer、Preferences、SPIFFS、LEDC | 以 [上游许可](https://github.com/espressif/arduino-esp32/blob/master/LICENSE.md) 为准 |
| ArduinoJson | `^6.21.3` | JSON 编解码 | [MIT](https://github.com/bblanchon/ArduinoJson/blob/6.x/LICENSE.md) |
| U8g2 | `^2.34.0` | 可选 SSD1306 显示实现 | [BSD-2-Clause](https://github.com/olikraus/u8g2/blob/master/LICENSE) |
| HX711 | `^0.7.5` | HX711 读取 | [BSD-3-Clause](https://github.com/bogde/HX711/blob/master/LICENSE) |

版本约束由包管理器解析；构建日志中的实际版本仅是本次构建证据，不构成上游长期支持承诺。

## Flutter 客户端

| 组件 | `app/pubspec.yaml` 约束 | 最近本机构建锁定版本 | 用途 | 许可来源 |
| :-- | :-- | :-- | :-- | :-- |
| Flutter / Dart SDK | SDK `>=3.0.0 <4.0.0` | Flutter `3.41.2` / Dart `3.11.0` | 客户端框架与工具链 | [BSD-3-Clause](https://github.com/flutter/flutter/blob/master/LICENSE) |
| provider | `^6.1.1` | `6.1.5+1` | 状态管理 | [MIT](https://github.com/rrousselGit/provider/blob/master/packages/provider/LICENSE) |
| http | `^1.1.0` | `1.6.0` | HTTP 客户端 | [BSD-3-Clause](https://github.com/dart-lang/http/blob/master/pkgs/http/LICENSE) |
| shared_preferences | `^2.2.2` | `2.5.4` | 本机保存设备地址 | [BSD-3-Clause](https://github.com/flutter/packages/blob/main/packages/shared_preferences/shared_preferences/LICENSE) |
| fl_chart | `^0.69.0` | `0.69.2` | 记录统计图表 | [MIT](https://github.com/imaNNeo/fl_chart/blob/main/LICENSE) |
| flutter_lints | `^3.0.0` | `3.0.2` | 静态分析规则 | [BSD-3-Clause](https://github.com/flutter/packages/blob/main/packages/flutter_lints/LICENSE) |

Android/Apple/Flutter 生成的项目骨架、Gradle wrapper 与插件注册文件保留在 Flutter scaffold 中，并各自受其上游许可证约束；不应把本仓库的 MIT 许可证误解为可替代这些上游许可证。

## 名称与硬件模块

ESP32、ESP、Arduino、PlatformIO、Flutter、HX711、U8g2、PIR、SPIFFS 等名称或商标归各自权利人所有。提及兼容模块不表示厂商背书、认证、医疗适用性或供货承诺。

## 未随仓库分发的材料

- 原始 `smart_pillbox-1.0.0-release.apk` 是历史构建产物，未纳入 Git；
- `.pio`、`.dart_tool`、`.gradle`、IDE 状态、Flutter ephemeral、编译输出和个人配置均未纳入 Git；
- 当前没有公开实物照片、视频、原理图、PCB、Gerber、制造文件或实测日志；
- 任何用户自行添加的药品、健康、网络或照片数据，都需要自行确认其隐私、授权和再分发边界。
