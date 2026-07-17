/// Shared defaults for device connection (single source of truth).
class AppConstants {
  AppConstants._();

  /// Host only (no scheme), for the IP text field in settings.
  /// `192.168.4.1` is the ESP32 AP provisioning address. It is intentionally
  /// only a neutral starting value: after the device joins Wi-Fi, enter its
  /// actual STA address before attempting REST operations.
  static const String defaultDeviceHost = '192.168.4.1';

  /// Default ESP32 HTTP base URL; user can override in settings.
  static String get defaultDeviceBaseUrl => 'http://$defaultDeviceHost';

  static const String prefsDeviceHostKey = 'device_ip_host';

  /// Validates an IPv4 dotted-quad host entered for the local device.
  static bool isValidIpv4(String host) {
    final trimmed = host.trim();
    final re = RegExp(
      r'^((25[0-5]|2[0-4]\d|[01]?\d\d?)\.){3}(25[0-5]|2[0-4]\d|[01]?\d\d?)$',
    );
    return re.hasMatch(trimmed);
  }
}
