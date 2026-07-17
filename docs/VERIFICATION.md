# 验证说明

> 状态日期：2026-07-17

## 一键本地门禁

```bash
bash scripts/verify.sh
```

门禁会在临时目录执行：

1. 敏感信息、私钥、真实 Wi-Fi、个人路径、生成物、APK 和缓存检查；
2. 必需文件、仓库结构、BOM、SVG、状态文案、PlatformIO 固定版本和安全边界检查；
3. 不依赖硬件的 Python 源码契约测试；
4. PlatformIO ESP32 固件干净构建；
5. `app/` 的 `flutter pub get`、`flutter test`、`flutter analyze`、`flutter build web --release`。

它不会烧录 ESP32、连接真实 Wi-Fi、输入真实密码、调用真实 REST、判断药物是否被服用，也不会改写权威来源目录。

## 已验证环境与结果

```text
PlatformIO Core: 6.1.19
Platform: espressif32 6.13.0
Board target: esp32dev
Flutter: 3.41.2
Dart: 3.11.0
```

2026-07-17 的隔离构建结果：

```text
Firmware RAM:   46588 / 327680 bytes (14.2%)
Firmware Flash: 896141 / 1310720 bytes (68.4%)
Flutter: widget test passed; `flutter analyze` returned `No issues found!`; release web build passed
```

一键门禁会在 Web 构建后移除 Flutter 临时写入的本机元数据，再执行末次敏感信息与仓库结构检查；这些生成物不属于候选提交。构建证据不证明 Flash 实际可用、烧录、SPIFFS、PIR、HX711、RGB、蜂鸣器、Wi-Fi、NTP、HTTP 或 App 端到端行为。

## 当前真机复测清单

后续复测必须记录日期、完整 Git commit、精确板型、模块/供电事实及每项通过、失败或未测：

- [ ] 确认 ESP32 开发板型号、Flash、USB 芯片与稳定供电；
- [ ] 确认两个 PIR、两个 HX711、称重结构、RGB、蜂鸣器和可选 OLED 的型号、供电、共地、逻辑电平与限流；
- [ ] 烧录当前 commit，记录串口启动、SPIFFS 挂载和错误路径；
- [ ] 无保存 Wi-Fi 时确认开放 AP、`http://192.168.4.1/`、配置保存与重启；
- [ ] 使用测试 Wi-Fi 完成 STA，记录实际 IP，但不要公开真实拓扑；
- [ ] 验证 NTP 成功、失败和断网路径；
- [ ] 验证 `/status`、`/plan`、`/records`、`/config`、`/remind`、`/restart` 和 STA 未知路径真实 `404`；
- [ ] 确认 `GET /config` 不泄露 SSID/密码，POST 配网后敏感字段不写入 SPIFFS `config.json`；
- [ ] 逐药仓验证 PIR 触发、HX711 读数、漂移、阈值、`take_ok`、`take_and_return`、`open_no_take`、`weight_without_motion` 和 `missed_dose` 的通过/失败情况；
- [ ] 验证 RGB 极性、限流、蜂鸣器驱动与提醒队列；
- [ ] 用 Android/iOS 真机或明确平台运行 Flutter App，检查地址配置、超时、错误态、计划与记录；
- [ ] 连续运行 30–60 分钟，记录重启、内存、Wi-Fi、SPIFFS、传感器漂移和 API 超时；
- [ ] 照片、视频、日志去除 EXIF/GPS、SSID、密码、私网拓扑、药品/健康信息、账号和设备唯一标识。

只有完成日期化、commit 绑定的复测，才能把“当前端到端真机复测未执行”升级为精确、可审计的结论；仍不得把原型状态机夸大为医疗判断。
