import 'dart:convert';
import 'package:http/http.dart' as http;
import '../constants.dart';

class PagedRecords {
  PagedRecords({required this.total, required this.items});
  final int total;
  final List<Map<String, dynamic>> items;
}

class ApiService {
  String baseUrl = AppConstants.defaultDeviceBaseUrl;

  static const _timeout = Duration(seconds: 5);

  void setBaseUrl(String url) {
    baseUrl = url;
  }

  Future<Map<String, dynamic>?> getStatus() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/status')).timeout(_timeout);
      if (response.statusCode == 200) {
        final body = response.body.trim();
        if (body.isEmpty) return null;
        final decoded = json.decode(body);
        if (decoded is Map<String, dynamic>) return decoded;
        return null;
      }
      return null;
    } catch (e) {
      rethrow;
    }
  }

  Future<Map<String, dynamic>?> getPlan() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/plan')).timeout(_timeout);
      if (response.statusCode == 200) {
        final body = response.body.trim();
        if (body.isEmpty) return {'plans': []};
        final decoded = json.decode(body);
        if (decoded is Map<String, dynamic>) return decoded;
        return {'plans': []};
      }
      return null;
    } catch (e) {
      rethrow;
    }
  }

  Future<bool> setPlan(Map<String, dynamic> plan) async {
    try {
      // No Content-Type header: ESP32 WebServer stores the raw body under
      // server.arg("plain"), which is the only reliable way to read POST body.
      final response = await http
          .post(
            Uri.parse('$baseUrl/plan'),
            body: json.encode(plan),
          )
          .timeout(_timeout);
      return response.statusCode == 200;
    } catch (e) {
      rethrow;
    }
  }

  /// 全量拉取（统计用）：返回 JSON 数组
  Future<List<dynamic>?> getRecords() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/records')).timeout(_timeout);
      if (response.statusCode == 200) {
        final body = response.body.trim();
        if (body.isEmpty) return [];
        final decoded = json.decode(body);
        if (decoded is List) return decoded;
        return [];
      }
      return null;
    } catch (e) {
      rethrow;
    }
  }

  Future<PagedRecords?> getRecordsPaged({int offset = 0, int limit = 50}) async {
    try {
      final uri = Uri.parse('$baseUrl/records').replace(
        queryParameters: {
          'offset': offset.toString(),
          'limit': limit.toString(),
        },
      );
      final response = await http.get(uri).timeout(_timeout);
      if (response.statusCode != 200) return null;
      final body = response.body.trim();
      if (body.isEmpty) return PagedRecords(total: 0, items: []);
      final decoded = json.decode(body);
      if (decoded is! Map<String, dynamic>) return PagedRecords(total: 0, items: []);
      final total = (decoded['total'] as num?)?.toInt() ?? 0;
      final raw = decoded['items'] as List<dynamic>? ?? [];
      final items = raw.map((e) => Map<String, dynamic>.from(e as Map)).toList();
      return PagedRecords(total: total, items: items);
    } catch (e) {
      rethrow;
    }
  }

  Future<bool> addRecord(Map<String, dynamic> record) async {
    try {
      final response = await http
          .post(
            Uri.parse('$baseUrl/records'),
            body: json.encode(record),
          )
          .timeout(_timeout);
      return response.statusCode == 200;
    } catch (e) {
      rethrow;
    }
  }

  Future<Map<String, dynamic>?> getConfig() async {
    try {
      final response = await http.get(Uri.parse('$baseUrl/config')).timeout(_timeout);
      if (response.statusCode == 200) {
        final body = response.body.trim();
        if (body.isEmpty) return {};
        final decoded = json.decode(body);
        if (decoded is Map<String, dynamic>) return decoded;
        return {};
      }
      return null;
    } catch (e) {
      rethrow;
    }
  }

  Future<bool> triggerReminder() async {
    try {
      final response = await http
          .post(Uri.parse('$baseUrl/remind'))
          .timeout(_timeout);
      return response.statusCode == 200;
    } catch (e) {
      rethrow;
    }
  }
}
