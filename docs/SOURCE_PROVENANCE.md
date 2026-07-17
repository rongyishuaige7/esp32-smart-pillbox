# 源码来源与权威副本裁决

> 状态日期：2026-07-17

## 只读来源

```text
/home/rongyi/桌面/ESP32智能药盒
/mnt/shared/2026项目/ESP32智能药盒.zip
```

历史 ZIP SHA-256：

```text
23d03a0e4c7c4297c36e04554ba4c300b35e2e31ab4c6091ab77ace628adcd0b
```

比较时排除 `.pio`、`.dart_tool`、`build`、`.gradle`、`.idea`、`.vscode`、Flutter `ephemeral`、`.iml`、APK、ELF/BIN/MAP 与其他生成/IDE 状态。按该规则，两个来源各有 162 个选定文件：

```text
桌面来源 manifest SHA-256: 207b8a40c1b978d502aff9adca60d9b385a4f668284a83cd293a70650315ce52
ZIP 来源 manifest SHA-256:  62f13bd53184f5b560533740c5fd4731f1fda681e86ee82bd0c781c0d09f943a
```

## 已确认差异与裁决

两个来源的固件选定文件完全一致。选定 Flutter 源码中只有以下一个内容差异：

```text
flutter_app/android/app/src/main/AndroidManifest.xml
```

桌面版本比历史 ZIP 增加：

- `android.permission.INTERNET`；
- `android.permission.ACCESS_NETWORK_STATE`；
- `android:usesCleartextTraffic="true"`。

这三个声明与 App 的局域网 HTTP 访问相匹配，属于桌面版本的后续 Android 网络整理；它们不证明设备、网络或 App 已重新真机测试。因此本轮裁决为：

- 历史 ZIP：只读封存基线；
- 桌面目录：本轮公开整理的权威源码来源；
- 公开候选：`/home/rongyi/桌面/esp32-smart-pillbox`；
- 原目录和 ZIP 始终只读，不由候选仓库反向覆盖、清理或删除。

## 可审计公开整理

在不改动原始来源的前提下，公开候选：

1. 将固件与 Flutter 客户端分别整理为 `firmware/` 与 `app/`；
2. 排除历史 APK、PlatformIO 缓存、Flutter/Gradle 构建状态、IDE 文件和其他生成物；
3. 将 App 默认地址改为 AP 配网起点 `http://192.168.4.1`，并明确它不是已连接 STA REST 设备的保证；
4. 将 UI 的连接文案收窄为“本次状态请求成功”，不把一次 HTTP 响应伪装成系统在线或健康状态；
5. 新增中文 README、BOM、源码推导接线边界图、协议、项目状态、验证说明、许可证、第三方声明、敏感信息门禁与 CI；
6. 保留 Android 明文 HTTP 声明，并明确 HTTP 无认证、无 TLS，仅限隔离可信局域网。

这些是公开前的可审计净化、文档与构建加固，不得表述为当前真机、传感器、提醒、网络、服药逻辑或 Flutter 端到端行为已验证。
