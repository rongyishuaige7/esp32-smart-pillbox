# ESP32 智能药盒

> **使用提示：** 本项目用于 ESP32、传感器和 Flutter 学习；不是医疗器械，不用于诊断、治疗、紧急呼叫、服药判定或替代人工照护。

基于 ESP32、两个 PIR、两个 HX711、蜂鸣器、RGB LED 与 Flutter 客户端的局域网智能药盒原型。它演示了：双药仓的人体接近与重量变化状态机、定时提醒、SPIFFS 本地记录、AP 配网与 Flutter 端的用药计划/记录界面。

## 项目资料

这里整理了项目照片、界面截图和相关资料；文件处理说明见 [MEDIA_EVIDENCE](docs/MEDIA_EVIDENCE.md)。

![ESP32 智能药盒原型，2026-04-08](assets/photos/historical-prototype.jpg)

## 功能概览（源码层）

```text
Flutter App ── HTTP（局域网）── ESP32 固件
                                  ├─ PIR × 2：接近/动作输入
                                  ├─ HX711 × 2：重量变化输入
                                  ├─ RGB + 蜂鸣器：提醒输出
                                  ├─ SPIFFS：计划、记录与配置
                                  └─ Wi-Fi：AP 配网 / STA 本地 REST
```

### 双药仓行为状态机

固件以 PIR 与 HX711 读数的组合区分以下**原型事件**：

- `take_ok`：拿起并放回后，重量变化超过源码阈值；
- `take_and_return`：拿起后原样放回；
- `open_no_take`：PIR 触发后未观察到足够重量变化；
- `weight_without_motion`：无 PIR 时出现的重量突变；
- `missed_dose`：提醒后，在固定时间窗内没有观察到 PIR 动作。

这些事件由当前固件的启发式状态机生成；阈值、传感器漂移和安装位置都会影响结果，不能作为医疗或实际服药结论。

## 仓库结构

```text
firmware/                  ESP32 PlatformIO + Arduino 固件
app/                       Flutter 客户端（Android/iOS/Linux/macOS/Web/Windows scaffold）
hardware/BOM.csv           器件与接口清单
hardware/wiring-diagram.svg 源码推导接线边界图
docs/                      来源、协议、状态、验证与 Hardware Lab 卡片
scripts/                   公开仓敏感信息/结构检查与一键验证
.github/workflows/         GitHub Actions 构建门禁
```

## 源码接口与硬件说明

| 信号 | 当前源码接口 | 说明 |
| :-- | :-- | :-- |
| 蜂鸣器 | GPIO18 | 由 LEDC 驱动；仅适用于低功率、正确驱动的器件。 |
| RGB | R=GPIO13、G=GPIO21、B=GPIO22 | 默认共阳低电平点亮；共阴模块需调整构建标志。 |
| PIR × 2 | GPIO15、GPIO25 | 药仓 2 使用 `INPUT_PULLDOWN`。 |
| HX711 × 2 | `DT/SCK = 4/2` 与 `33/32` | 当前阈值仅为原型代码参数，不是剂量标定。 |
| 可选 SSD1306 | SDA=GPIO26、SCL=GPIO27 | 有实现文件，但本提交 `main.cpp` 没有调用显示初始化/刷新。 |

完整限制见 [HARDWARE.md](HARDWARE.md)。请先确认模块电压、供电、共地、限流、量程与真实板型；不要把 5 V 直接接入 ESP32 GPIO，也不要直接驱动继电器、电机、市电或大功率负载。

## 网络与数据安全

### AP 配网

没有已保存的 Wi-Fi 配置或 STA 连接失败时，固件进入 AP 模式：

```text
SSID: SmartPillbox
Password: none (open AP)
Config URL: http://192.168.4.1/
```

这是公开教学配置，并非安全凭据。AP 与 REST 都是**无认证、无 TLS 的 HTTP**，只能在隔离、可信的本地测试网络中使用，绝不能暴露到公网。

### STA REST

设备成功加入 Wi-Fi 后，WebServer 在端口 `80` 提供本地 HTTP 接口。App 默认地址 `http://192.168.4.1` 只用于进入配网起点；此地址不是已连接 STA 设备的保证。完成配网后，必须在 App 中手动填写设备实际获得的 STA IP。

当前接口包括：

| 方法 | 路径 | 当前源码行为 |
| :-- | :-- | :-- |
| `GET` | `/status` | 当前运行时间、IP、PIR 与提醒标志。 |
| `GET` / `POST` | `/plan` | 读取/写入计划 JSON。 |
| `GET` / `POST` | `/records` | 读取/追加事件记录；支持 `offset` / `limit` 分页。 |
| `GET` / `POST` | `/config` | 读取/写入配置；POST 中的 Wi-Fi 字段会写入设备本地 Preferences。 |
| `POST` | `/remind` | 触发一次源码中的提醒队列。 |
| `POST` | `/restart` | 返回成功后重启设备。 |

固件对未知路径在 AP 模式重定向到配网页，在 STA 模式返回 JSON `404`。CORS 为 `Access-Control-Allow-Origin: *`。
> **敏感数据提醒：** 不要调用 `GET /config` 后公开响应，也不要在 Issue、日志、截图、视频或 Git 中泄露 Wi-Fi SSID、密码、私网 IP、个人服药记录或其他健康/身份信息。

## 构建

### ESP32 固件

```bash
cd firmware
pio run
```

已验证环境：PlatformIO Core `6.1.19`、`espressif32@6.13.0`、`esp32dev`。最近一次构建结果为 RAM `46588 / 327680`（14.2%）和 Flash `896141 / 1310720`（68.4%）。

### Flutter 客户端

```bash
cd app
flutter pub get
flutter test
flutter analyze
flutter build web --release
```

已验证环境：Flutter `3.41.2`、Dart `3.11.0`。最近一次门禁中，Flutter `analyze` 返回 `No issues found!`。

### 一键门禁

```bash
bash scripts/verify.sh
```

## 许可证、第三方与学习使用

- 本仓库中 Rongyi 原创固件、Flutter 客户端与文档以 [MIT License](LICENSE) 公开；
- 第三方依赖和框架见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)；
- 本项目适用于阅读、学习、实验和二次开发参考；请不要将代码、文档或演示直接包装为个人课程设计、毕业设计、竞赛或医疗产品成果；
- 复用时请保留来源说明，并自行承担硬件、电气、网络、数据与适用性验证责任。

## 更多资料

- [来源裁决](docs/SOURCE_PROVENANCE.md)
- [协议说明](docs/PROTOCOL.md)
- [验证说明](docs/VERIFICATION.md)
- [Hardware Lab 索引卡片](docs/HARDWARE_LAB_CARD.md)
- [Hardware Lab](https://github.com/rongyishuaige7/hardware-lab)
