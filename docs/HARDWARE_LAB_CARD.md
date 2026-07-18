# Hardware Lab 索引卡片草案

## 名称

```text
ESP32 智能药盒
```

## 摘要

```text
基于 ESP32、双 PIR、双 HX711、RGB/蜂鸣器与 Flutter 局域网客户端的智能药盒教学原型。
```

## 平台

```text
ESP32 · Arduino · PlatformIO · Flutter · PIR · HX711 · SPIFFS · HTTP
```

## 真实状态口径

```text
源码来源、硬件无关源码契约、ESP32 固件与 Flutter 客户端构建已验证；当前 ESP32、两个 PIR、两个 HX711、RGB、蜂鸣器、SPIFFS、Wi-Fi、NTP 与 Flutter App 端到端链路尚未重新真机复测。
```

## 公开范围与边界

```text
当前没有公开实物照片、演示视频、原理图、PCB、Gerber 或制造文件；公开 BOM、源码推导接线边界图、协议、来源与验证说明。HTTP 无认证、无 TLS，仅限隔离可信局域网；原型状态机不构成服药、诊断或医疗结论。
```

填入 Hardware Lab 前必须将本卡的 `head_sha`、固定 Actions URL 与最终公开仓 exact HEAD 同步；未完成线上 CI 前不得写成构建证据已通过。

- **项目素材：** 已补充项目照片、界面截图和相关资料；范围和版本差异见 [`MEDIA_EVIDENCE.md`](MEDIA_EVIDENCE.md)。
