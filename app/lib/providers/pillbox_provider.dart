import 'package:flutter/foundation.dart';
import 'package:shared_preferences/shared_preferences.dart';
import '../constants.dart';
import '../services/api_service.dart';

class PillboxProvider extends ChangeNotifier {
  PillboxProvider() {
    _loadSavedDeviceUrl();
  }

  final ApiService _apiService = ApiService();

  bool isConnected = false;
  String ipAddress = '';
  String deviceStatus = '';
  int uptime = 0;
  bool sensor1Motion = false;
  bool sensor2Motion = false;
  bool reminderActive = false;
  bool missedPending1 = false;
  bool missedPending2 = false;
  String? lastError;

  List<Map<String, dynamic>> plans = [];
  List<Map<String, dynamic>> records = [];
  int recordsTotal = 0;

  /// Full list for statistics (separate from paged [records] to avoid cross-tab corruption).
  List<Map<String, dynamic>> statsRecords = [];

  /// Tracks concurrent network operations; true when any of [checkStatus], [loadPlans],
  /// [loadRecordsPage], [loadStatsRecords] is in flight (supports parallel [Future.wait]).
  int _loadingDepth = 0;
  bool get isLoading => _loadingDepth > 0;

  void _beginLoading() {
    _loadingDepth++;
    if (_loadingDepth == 1) notifyListeners();
  }

  void _endLoading() {
    if (_loadingDepth > 0) _loadingDepth--;
    if (_loadingDepth == 0) notifyListeners();
  }

  Future<void> _loadSavedDeviceUrl() async {
    try {
      final p = await SharedPreferences.getInstance();
      final host = p.getString(AppConstants.prefsDeviceHostKey);
      if (host != null && host.isNotEmpty && AppConstants.isValidIpv4(host)) {
        _apiService.setBaseUrl('http://$host');
        notifyListeners();
      }
    } catch (_) {}
  }

  Future<void> saveDeviceHost(String ipv4Host) async {
    final p = await SharedPreferences.getInstance();
    await p.setString(AppConstants.prefsDeviceHostKey, ipv4Host);
  }

  void clearError() {
    lastError = null;
    notifyListeners();
  }

  void setBaseUrl(String url) {
    _apiService.setBaseUrl(url);
  }

  String get baseUrl => _apiService.baseUrl;

  /// When [trackLoading] is true, updates [isLoading] for toolbar / pull-to-refresh feedback.
  /// Background polling and silent reloads should use the default `trackLoading: false`.
  Future<void> checkStatus({bool trackLoading = false}) async {
    if (trackLoading) _beginLoading();
    try {
      lastError = null;
      final status = await _apiService.getStatus();
      if (status != null) {
        // This only means the configured HTTP endpoint returned a status
        // response. It is not a cloud/service-health claim.
        isConnected = true;
        ipAddress = status['ip'] ?? '';
        deviceStatus = status['status'] ?? '';
        uptime = (status['uptime'] as num?)?.toInt() ?? 0;
        sensor1Motion = status['sensor1_motion'] == true;
        sensor2Motion = status['sensor2_motion'] == true;
        reminderActive = status['reminder_active'] == true;
        missedPending1 = status['missed_pending_1'] == true;
        missedPending2 = status['missed_pending_2'] == true;
      } else {
        isConnected = false;
      }
      notifyListeners();
    } catch (e) {
      isConnected = false;
      lastError = e.toString();
      notifyListeners();
    } finally {
      if (trackLoading) _endLoading();
    }
  }

  Future<void> loadPlans({bool trackLoading = false}) async {
    if (trackLoading) _beginLoading();
    try {
      lastError = null;
      final plan = await _apiService.getPlan();
      if (plan != null && plan['plans'] != null) {
        plans = List<Map<String, dynamic>>.from(plan['plans']);
      }
      notifyListeners();
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
    } finally {
      if (trackLoading) _endLoading();
    }
  }

  /// Refreshes device status and medication plans in parallel (single loading cycle for UI).
  Future<void> refreshAll({bool trackLoading = true}) async {
    await Future.wait([
      checkStatus(trackLoading: trackLoading),
      loadPlans(trackLoading: trackLoading),
    ]);
  }

  Future<bool> addPlan(Map<String, dynamic> singlePlan) async {
    try {
      lastError = null;
      final existing = await _apiService.getPlan();
      final List<Map<String, dynamic>> list = existing != null && existing['plans'] != null
          ? List<Map<String, dynamic>>.from(existing['plans'])
          : [];
      list.add(Map<String, dynamic>.from(singlePlan));
      final ok = await _apiService.setPlan({'plans': list});
      if (ok) await loadPlans();
      notifyListeners();
      return ok;
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
      return false;
    }
  }

  Future<bool> updatePlanEnabled(int index, bool enabled) async {
    if (index < 0 || index >= plans.length) return false;
    try {
      lastError = null;
      final list = List<Map<String, dynamic>>.from(plans);
      list[index] = Map<String, dynamic>.from(list[index])..['enabled'] = enabled;
      final ok = await _apiService.setPlan({'plans': list});
      if (ok) await loadPlans();
      notifyListeners();
      return ok;
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
      return false;
    }
  }

  Future<bool> deletePlan(int index) async {
    if (index < 0 || index >= plans.length) return false;
    try {
      lastError = null;
      final list = List<Map<String, dynamic>>.from(plans)..removeAt(index);
      final ok = await _apiService.setPlan({'plans': list});
      if (ok) await loadPlans();
      notifyListeners();
      return ok;
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
      return false;
    }
  }

  /// Loads all records for the statistics tab only (does not touch paged [records]).
  Future<void> loadStatsRecords({bool trackLoading = false}) async {
    if (trackLoading) _beginLoading();
    try {
      lastError = null;
      final result = await _apiService.getRecords();
      if (result != null) {
        statsRecords = List<Map<String, dynamic>>.from(result);
      }
      notifyListeners();
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
    } finally {
      if (trackLoading) _endLoading();
    }
  }

  Future<void> loadRecordsPage({int offset = 0, int limit = 50, bool trackLoading = false}) async {
    if (trackLoading) _beginLoading();
    try {
      lastError = null;
      final paged = await _apiService.getRecordsPaged(offset: offset, limit: limit);
      if (paged != null) {
        if (offset == 0) {
          records = paged.items;
        } else {
          records = [...records, ...paged.items];
        }
        recordsTotal = paged.total;
        _sortRecordsNewestFirst();
      }
      notifyListeners();
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
    } finally {
      if (trackLoading) _endLoading();
    }
  }

  /// Reloads paged records and full stats list in parallel.
  Future<void> refreshRecords({int offset = 0, int limit = 50, bool trackLoading = true}) async {
    await Future.wait([
      loadRecordsPage(offset: offset, limit: limit, trackLoading: trackLoading),
      loadStatsRecords(trackLoading: trackLoading),
    ]);
  }

  static DateTime? _parseRecordTime(Map<String, dynamic> r) {
    final s = r['time'] ?? r['date'] ?? r['actual_time'];
    if (s is! String || s.length < 16) return null;
    try {
      final p = s.split(RegExp(r'[\sT]'));
      if (p.length < 2) return null;
      final d = p[0].split('-');
      final t = p[1].split(':');
      return DateTime(
        int.parse(d[0]),
        int.parse(d[1]),
        int.parse(d[2]),
        int.parse(t[0]),
        int.parse(t[1]),
        t.length > 2 ? int.parse(t[2].length >= 2 ? t[2].substring(0, 2) : t[2]) : 0,
      );
    } catch (_) {
      return null;
    }
  }

  void _sortRecordsNewestFirst() {
    records.sort((a, b) {
      final ta = _parseRecordTime(a);
      final tb = _parseRecordTime(b);
      if (ta == null && tb == null) return 0;
      if (ta == null) return 1;
      if (tb == null) return -1;
      return tb.compareTo(ta);
    });
  }

  Future<bool> addRecord(Map<String, dynamic> record) async {
    try {
      lastError = null;
      final ok = await _apiService.addRecord(record);
      if (ok) await loadStatsRecords();
      notifyListeners();
      return ok;
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
      return false;
    }
  }

  Future<bool> triggerReminder() async {
    try {
      lastError = null;
      final ok = await _apiService.triggerReminder();
      notifyListeners();
      return ok;
    } catch (e) {
      lastError = e.toString();
      notifyListeners();
      return false;
    }
  }
}
