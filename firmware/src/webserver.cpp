#include "webserver.h"
#include "storage.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include "sensor.h"
#include "reminder.h"
#include "pillbox_status.h"
#include <ArduinoJson.h>
#include <HTTP_Method.h>

// 配网页面 HTML（AP 模式下访问 http://192.168.4.1 即可看到）
static const char kConfigPage[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>智能药盒 WiFi 配置</title>
<style>
  body{font-family:sans-serif;max-width:400px;margin:40px auto;padding:0 16px;background:#f5f5f5}
  h2{color:#1976D2;text-align:center}
  .card{background:#fff;border-radius:8px;padding:24px;box-shadow:0 2px 8px rgba(0,0,0,.12)}
  label{display:block;margin-bottom:4px;color:#555;font-size:.9em}
  input{width:100%;box-sizing:border-box;padding:10px;border:1px solid #ccc;border-radius:4px;font-size:1em;margin-bottom:16px}
  button{width:100%;padding:12px;background:#1976D2;color:#fff;border:none;border-radius:4px;font-size:1em;cursor:pointer}
  button:active{background:#1565C0}
  #msg{margin-top:12px;text-align:center;color:#388E3C;font-size:.9em}
</style>
</head>
<body>
<h2>&#128137; 智能药盒配网</h2>
<div class="card">
  <form id="f">
    <label>WiFi 名称（SSID）</label>
    <input type="text" id="ssid" name="ssid" placeholder="你家 WiFi 名称" required>
    <label>WiFi 密码</label>
    <input type="password" id="pw" name="pw" placeholder="WiFi 密码">
    <button type="submit">保存并重启</button>
  </form>
  <div id="msg"></div>
</div>
<script>
document.getElementById('f').addEventListener('submit',async function(e){
  e.preventDefault();
  const ssid=document.getElementById('ssid').value.trim();
  const pw=document.getElementById('pw').value;
  if(!ssid){document.getElementById('msg').textContent='请填写 WiFi 名称';return;}
  document.getElementById('msg').textContent='正在保存...';
  try{
    const r=await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},
      body:JSON.stringify({wifi_ssid:ssid,wifi_password:pw})});
    const j=await r.json();
    if(j.success){
      document.getElementById('msg').textContent='保存成功！设备将在 2 秒后重启，请连回你的 WiFi 再打开 App。';
      setTimeout(()=>fetch('/restart',{method:'POST'}),500);
    }else{
      document.getElementById('msg').textContent='保存失败，请重试。';
    }
  }catch(err){
    document.getElementById('msg').textContent='网络错误：'+err;
  }
});
</script>
</body>
</html>
)rawhtml";

WebServer server(80);

static void sendCors() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

static void handleOptions() {
    sendCors();
    server.send(204);
}

void handleStatus() {
    sendCors();
    StaticJsonDocument<512> doc;
    // A successful response only confirms that this local HTTP handler ran.
    // It does not assert cloud connectivity, sensor health, or medication use.
    doc["status"] = "local_response";
    doc["ip"] = wifiManager.getIP();
    doc["uptime"] = millis();
    doc["sensor1_motion"] = sensorManager.isMotionDetected(1);
    doc["sensor2_motion"] = sensorManager.isMotionDetected(2);
    doc["reminder_active"] = reminderManager.isActive();
    doc["missed_pending_1"] = pillboxMissedPending(1);
    doc["missed_pending_2"] = pillboxMissedPending(2);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleGetPlan() {
    sendCors();
    String plan = storageManager.getPlan();
    server.send(200, "application/json", plan);
}

void handleSetPlan() {
    sendCors();
    // server.arg("plain") works when no Content-Type or text/plain is sent.
    // As a fallback also read directly from the client stream.
    String body = server.arg("plain");
    if (body.length() == 0) {
        // Try reading remaining bytes directly from the underlying client
        WiFiClient client = server.client();
        while (client.available()) {
            body += (char)client.read();
        }
    }
    body.trim();
    if (body.length() > 0) {
        if (storageManager.savePlan(body)) {
            server.send(200, "application/json", "{\"success\":true}");
        } else {
            server.send(500, "application/json", "{\"success\":false}");
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
    }
}

void handleGetRecords() {
    sendCors();
    if (server.hasArg("offset") || server.hasArg("limit")) {
        size_t offset = server.hasArg("offset") ? (size_t)server.arg("offset").toInt() : 0;
        size_t limit = server.hasArg("limit") ? (size_t)server.arg("limit").toInt() : 50;
        if (limit == 0 || limit > 200) {
            limit = 50;
        }
        size_t total = 0;
        String body = storageManager.getRecordsPaged(offset, limit, &total);
        server.send(200, "application/json", body);
    } else {
        String records = storageManager.getRecords();
        server.send(200, "application/json", records);
    }
}

void handleAddRecord() {
    sendCors();
    String body = server.arg("plain");
    if (body.length() == 0) {
        WiFiClient client = server.client();
        while (client.available()) {
            body += (char)client.read();
        }
    }
    body.trim();
    if (body.length() > 0) {
        if (storageManager.saveRecord(body)) {
            server.send(200, "application/json", "{\"success\":true}");
        } else {
            server.send(500, "application/json", "{\"success\":false}");
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
    }
}

void handleGetConfig() {
    sendCors();
    // Wi-Fi credentials live in Preferences and must never be returned over
    // this unauthenticated HTTP API. The endpoint intentionally exposes only
    // a non-sensitive capability hint, rather than the raw saved JSON.
    StaticJsonDocument<128> doc;
    doc["wifi_configured"] = wifiManager.hasSavedConfig();
    doc["network_policy"] = "local_http_only";
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void handleSetConfig() {
    sendCors();
    String body = server.arg("plain");
    if (body.length() == 0) {
        WiFiClient client = server.client();
        while (client.available()) {
            body += (char)client.read();
        }
    }
    body.trim();
    if (body.length() > 0) {
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }

        if (doc.containsKey("wifi_ssid") && doc.containsKey("wifi_password")) {
            String ssid = doc["wifi_ssid"].as<String>();
            String password = doc["wifi_password"].as<String>();
            wifiManager.saveConfig(ssid.c_str(), password.c_str());
        }

        // Avoid duplicating credentials from a provisioning request in the
        // SPIFFS config file. Preferences is the single local storage owner.
        doc.remove("wifi_ssid");
        doc.remove("wifi_password");
        String sanitized;
        serializeJson(doc, sanitized);

        if (storageManager.saveConfig(sanitized)) {
            server.send(200, "application/json", "{\"success\":true}");
        } else {
            server.send(500, "application/json", "{\"success\":false}");
        }
    } else {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
    }
}

void handleTriggerReminder() {
    sendCors();
    reminderManager.startBlink(3, 500);
    server.send(200, "application/json", "{\"success\":true}");
}

// 配网首页
static void handleConfigPage() {
    server.send_P(200, "text/html; charset=utf-8", kConfigPage);
}

// 重启接口（配网页面保存后调用）
static void handleRestart() {
    sendCors();
    server.send(200, "application/json", "{\"success\":true}");
    delay(500);
    ESP.restart();
}

void handleNotFound() {
    // AP 模式下把任意未知 URL 重定向到配网页
    if (WiFi.getMode() == WIFI_AP) {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
        return;
    }
    sendCors();
    server.send(404, "application/json", "{\"error\":\"Not found\"}");
}

void HttpServer::begin() {
    server.on("/", HTTP_GET, handleConfigPage);
    server.on("/restart", HTTP_OPTIONS, handleOptions);
    server.on("/restart", HTTP_POST, handleRestart);
    server.on("/status", HTTP_OPTIONS, handleOptions);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/plan", HTTP_OPTIONS, handleOptions);
    server.on("/plan", HTTP_GET, handleGetPlan);
    server.on("/plan", HTTP_POST, handleSetPlan);
    server.on("/records", HTTP_OPTIONS, handleOptions);
    server.on("/records", HTTP_GET, handleGetRecords);
    server.on("/records", HTTP_POST, handleAddRecord);
    server.on("/config", HTTP_OPTIONS, handleOptions);
    server.on("/config", HTTP_GET, handleGetConfig);
    server.on("/config", HTTP_POST, handleSetConfig);
    server.on("/remind", HTTP_OPTIONS, handleOptions);
    server.on("/remind", HTTP_POST, handleTriggerReminder);

    server.onNotFound([]() {
        if (server.method() == HTTP_OPTIONS) {
            handleOptions();
            return;
        }
        handleNotFound();
    });

    server.begin();
    Serial.println("WebServer started on port 80");
}

void HttpServer::handle() {
    server.handleClient();
}

HttpServer httpServer;
