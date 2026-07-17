from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding='utf-8')


class SourceContracts(unittest.TestCase):
    def test_platform_and_source_gpio_contract(self):
        self.assertIn('platform = espressif32@6.13.0', read('firmware/platformio.ini'))
        main = read('firmware/src/main.cpp')
        self.assertIn('reminderManager.begin(18, 13, 21, 22)', main)
        self.assertIn('sensorManager.begin(15, 25, 4, 2, 33, 32)', main)
        display = read('firmware/src/display.cpp')
        self.assertIn('#define OLED_SDA 26', display)
        self.assertIn('#define OLED_SCL 27', display)

    def test_ap_is_open_and_rest_boundary_is_explicit(self):
        wifi = read('firmware/src/wifi_manager.cpp')
        protocol = read('docs/PROTOCOL.md')
        self.assertIn('WiFi.softAP(ap_ssid)', wifi)
        self.assertNotIn('ap_password', wifi)
        self.assertIn('SSID: SmartPillbox', protocol)
        self.assertIn('没有 TLS、认证', protocol)

    def test_status_does_not_claim_system_online(self):
        server = read('firmware/src/webserver.cpp')
        home = read('app/lib/pages/home_page.dart')
        self.assertIn('doc["status"] = "local_response"', server)
        self.assertNotIn('doc["status"] = "online"', server)
        self.assertIn("'本次状态请求成功'", home)
        self.assertNotIn("'设备在线'", home)

    def test_config_get_is_redacted_and_post_strips_credentials_before_spiffs(self):
        server = read('firmware/src/webserver.cpp')
        self.assertIn('doc["wifi_configured"]', server)
        self.assertIn('doc.remove("wifi_ssid")', server)
        self.assertIn('doc.remove("wifi_password")', server)
        self.assertNotIn('String config = storageManager.getConfig();', server)

    def test_sta_default_is_neutral_ap_start_not_fake_connected_device(self):
        constants = read('app/lib/constants.dart')
        self.assertIn("defaultDeviceHost = '192.168.4.1'", constants)
        self.assertIn('actual STA address', constants)
        self.assertIn('android:usesCleartextTraffic="true"', read('app/android/app/src/main/AndroidManifest.xml'))

    def test_widget_test_tracks_visible_title(self):
        self.assertIn("find.text('智能药盒')", read('app/test/widget_test.dart'))
        self.assertIn("const Text('智能药盒'", read('app/lib/pages/home_page.dart'))


if __name__ == '__main__':
    unittest.main()
