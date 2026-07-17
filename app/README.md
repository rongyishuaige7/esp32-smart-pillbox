# 智能药盒 Flutter 客户端

与 ESP32 固件通过局域网 HTTP API 通信。默认 `http://192.168.4.1` 只作为 AP 配网入口；设备加入 Wi-Fi 后，必须在设置页填写实际 STA IPv4 地址。

客户端使用无认证、无 TLS 的本地 HTTP，仅适用于隔离可信网络。一次状态请求成功不等于设备在线、传感器准确、提醒送达或服药完成。完整边界见根目录 [README](../README.md)。

## 功能

- **首页**：连接状态、传感器与漏服监测提示、用药计划、测试提醒
- **计划**：增删改用药计划（含药仓）
- **记录**：服药/行为记录，支持分页「加载更多」
- **统计**：依从性比例、`take_ok` / 漏服等计数、近 7 日柱状图
- **设置**：设备 IP（使用 `shared_preferences` 持久化）、Wi‑Fi 配网

## API 约定

- `GET /status`：含 `missed_pending_1`、`missed_pending_2`
- `GET /records`：返回 JSON 数组（兼容旧版）
- `GET /records?offset=0&limit=50`：返回 `{"total":N,"items":[...]}`

## 运行

```bash
flutter pub get
flutter run -d chrome   # 或连接的设备
```
