# 配网与局域网 REST 协议


## 网络阶段

### 阶段 A：AP 配网

当 Preferences 中没有已保存的 SSID，或 STA 自动连接失败时，固件创建开放 AP：

```text
SSID: SmartPillbox
配置页: http://192.168.4.1/
```

AP 刻意没有密码，避免在公开源代码中伪装一条“私有”固定密码。它只能用于短时间、隔离、可信的本地配置；完成配置后应让设备退出测试网络或断电。页面将 SSID 与密码 POST 到 `/config`，固件写入设备本地 Preferences，然后可调用 `/restart`。

### 阶段 B：STA + REST

设备成功加入用户 Wi-Fi 后，`WebServer` 在端口 `80` 提供 REST。手机/电脑必须和设备处于同一个隔离可信局域网。App 初始的 `http://192.168.4.1` 只是 AP 配网起点；完成配网后必须填写实际 STA IP。

HTTP 没有 TLS、认证、会话、授权、设备身份或请求签名。固件开放：

```text
Access-Control-Allow-Origin: *
Access-Control-Allow-Methods: GET, POST, OPTIONS
Access-Control-Allow-Headers: Content-Type
```

因此它不能用于公网、不可信网络或承载真实敏感健康数据。

## 端点

| 方法 | 路径 | 当前源码行为 | 安全/语义边界 |
| :-- | :-- | :-- | :-- |
| `GET` | `/status` | 返回 `status: local_response`、IP、运行时间、PIR 和提醒标志。 | 成功仅说明该 HTTP handler 在当前请求中返回；不是设备健康、云服务、传感器准确或服药完成证明。 |
| `GET` | `/plan` | 读取 SPIFFS `plan.json`。 | 未认证；不要把个人计划暴露在不可信网络。 |
| `POST` | `/plan` | 原样写入计划 JSON。 | 目前没有 schema/长度/身份校验；只限受控教学环境。 |
| `GET` | `/records` | 返回记录数组；可带 `offset` / `limit`。 | 原型记录可能含个人用药行为，不应公开。 |
| `POST` | `/records` | 追加一条记录。 | 没有认证或严格 schema；最大保留 200 条由源码逻辑控制。 |
| `GET` | `/config` | 仅返回 `wifi_configured` 和网络策略提示。 | 不返回 SSID 或密码。 |
| `POST` | `/config` | 处理配置 JSON；若含 Wi-Fi 字段，写入 Preferences；敏感字段不会再写入 SPIFFS `config.json`。 | 请求仍为明文 HTTP；只能在可信 AP/局域网使用。 |
| `POST` | `/remind` | 触发源码中的 RGB/蜂鸣器提醒队列。 | HTTP 成功不证明物理输出实际发生。 |
| `POST` | `/restart` | 返回成功后调用 ESP 重启。 | 未认证；不得暴露公网。 |
| `OPTIONS` | 已注册路径/未知路径 | 返回 CORS 预检 `204`。 | 仅浏览器兼容行为。 |

未知路径：AP 模式重定向到 `http://192.168.4.1/`；STA 模式返回：

```text
HTTP 404
{"error":"Not found"}
```

## 客户端状态语义

Flutter App 使用 5 秒超时访问上述 API。设置页保存的是用户输入的主机地址；首页的“本次状态请求成功”只表示 `GET /status` 得到可解析响应。它不等于设备在线、提醒已送达、药物已服用、网络安全、传感器已验证或长时间稳定。

## 公开/日志禁止项

不要公开：Wi-Fi SSID、Wi-Fi 密码、实际 STA IP、MAC、SPIFFS 导出、个人用药计划、记录、家庭网络拓扑、手机标识、截图 EXIF/GPS 或视频中可识别的个人信息。
