import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/pillbox_provider.dart';

/// Parses ESP32 record time strings like `2026-03-29 17:05:23`.
DateTime? _tryParseRecordTime(String s) {
  final t = s.trim();
  if (t.isEmpty) return null;
  try {
    if (t.contains(' ') && !t.contains('T')) {
      return DateTime.parse(t.replaceFirst(' ', 'T'));
    }
    return DateTime.parse(t);
  } catch (_) {
    return null;
  }
}

/// Friendly Chinese display, e.g. `03月29日 17:05`. Falls back to raw string if parse fails.
String _formatRecordTimeDisplay(dynamic raw) {
  if (raw == null) return '未知时间';
  final s = raw.toString().trim();
  if (s.isEmpty) return '未知时间';
  final dt = _tryParseRecordTime(s);
  if (dt == null) return s;
  return '${dt.month.toString().padLeft(2, '0')}月${dt.day.toString().padLeft(2, '0')}日 '
      '${dt.hour.toString().padLeft(2, '0')}:${dt.minute.toString().padLeft(2, '0')}';
}

String _behaviorLabel(String? b) {
  switch (b) {
    case 'take_ok':
      return '按时服药';
    case 'take_and_return':
      return '取药未服';
    case 'open_no_take':
      return '开盖未取';
    case 'weight_without_motion':
      return '称重异常';
    case 'missed_dose':
      return '漏服';
    default:
      return b ?? '—';
  }
}

class _RowVisual {
  const _RowVisual({
    required this.color,
    required this.icon,
    required this.trailing,
  });
  final Color color;
  final IconData icon;
  final String trailing;
}

_RowVisual _rowVisualFor(Map<String, dynamic> record) {
  final b = record['behavior'] as String?;
  final isMissed = (record['status'] ?? '').toString() == 'missed' || b == 'missed_dose';
  if (isMissed) {
    return const _RowVisual(
      color: Colors.deepOrange,
      icon: Icons.priority_high,
      trailing: '漏服',
    );
  }
  switch (b) {
    case 'take_ok':
      return const _RowVisual(
        color: Colors.green,
        icon: Icons.check,
        trailing: '已服药',
      );
    case 'take_and_return':
      return const _RowVisual(
        color: Colors.orange,
        icon: Icons.undo,
        trailing: '取药未服',
      );
    case 'open_no_take':
      return _RowVisual(
        color: Colors.red.shade700,
        icon: Icons.folder_open,
        trailing: '开盖未取',
      );
    case 'weight_without_motion':
      return const _RowVisual(
        color: Colors.grey,
        icon: Icons.warning_amber_outlined,
        trailing: '称重异常',
      );
    default:
      final taken = record['taken'] == true || (record['status'] ?? '').toString() == 'taken';
      if (taken) {
        return const _RowVisual(
          color: Colors.green,
          icon: Icons.check,
          trailing: '已服药',
        );
      }
      return const _RowVisual(
        color: Colors.red,
        icon: Icons.close,
        trailing: '未服药',
      );
  }
}

class _StatsData {
  _StatsData({
    required this.takeOk,
    required this.takeAndReturn,
    required this.openNoTake,
    required this.missed,
    required this.other,
  });
  final int takeOk;
  final int takeAndReturn;
  final int openNoTake;
  final int missed;
  final int other;
  int get total => takeOk + takeAndReturn + openNoTake + missed + other;
}

_StatsData _computeStats(List<Map<String, dynamic>> records) {
  var takeOk = 0;
  var takeAndReturn = 0;
  var openNoTake = 0;
  var missed = 0;
  var other = 0;
  for (final r in records) {
    final b = r['behavior'] as String? ?? '';
    final status = (r['status'] ?? '').toString();
    final isMissed = b == 'missed_dose' || status == 'missed';
    final legacyTaken = r['taken'] == true && b.isEmpty && !isMissed;
    if (isMissed) {
      missed++;
    } else if (b == 'take_ok' || legacyTaken) {
      takeOk++;
    } else if (b == 'take_and_return') {
      takeAndReturn++;
    } else if (b == 'open_no_take') {
      openNoTake++;
    } else {
      other++;
    }
  }
  return _StatsData(
    takeOk: takeOk,
    takeAndReturn: takeAndReturn,
    openNoTake: openNoTake,
    missed: missed,
    other: other,
  );
}

class RecordPage extends StatefulWidget {
  const RecordPage({super.key});

  @override
  State<RecordPage> createState() => _RecordPageState();
}

class _RecordPageState extends State<RecordPage> with SingleTickerProviderStateMixin {
  static const _pageSize = 40;
  bool _loadingMore = false;
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final p = context.read<PillboxProvider>();
      p.refreshRecords(offset: 0, limit: _pageSize, trackLoading: false);
    });
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  Future<void> _loadMore() async {
    final p = context.read<PillboxProvider>();
    if (p.records.length >= p.recordsTotal || _loadingMore) return;
    setState(() => _loadingMore = true);
    await p.loadRecordsPage(offset: p.records.length, limit: _pageSize);
    if (mounted) setState(() => _loadingMore = false);
  }

  Future<void> _onRefresh() async {
    final p = context.read<PillboxProvider>();
    await p.refreshRecords(offset: 0, limit: _pageSize);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('记录与统计', style: TextStyle(fontWeight: FontWeight.bold)),
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(text: '服药记录'),
            Tab(text: '用药统计'),
          ],
        ),
        actions: [
          Consumer<PillboxProvider>(
            builder: (context, provider, _) {
              return IconButton(
                tooltip: '刷新',
                icon: provider.isLoading
                    ? const SizedBox(
                        width: 24,
                        height: 24,
                        child: CircularProgressIndicator(strokeWidth: 2),
                      )
                    : const Icon(Icons.refresh),
                onPressed: provider.isLoading ? null : () => _onRefresh(),
              );
            },
          ),
        ],
      ),
      body: TabBarView(
        controller: _tabController,
        children: [
          _RecordsListTab(
            loadingMore: _loadingMore,
            onLoadMore: _loadMore,
            onRefresh: _onRefresh,
          ),
          Consumer<PillboxProvider>(
            builder: (context, provider, _) {
              if (provider.lastError != null && provider.statsRecords.isEmpty) {
                return Center(child: Text('加载失败: ${provider.lastError}'));
              }
              final s = _computeStats(provider.statsRecords);
              final total = s.total;
              return ListView(
                padding: const EdgeInsets.all(16),
                children: [
                  _StatCard(
                    label: '按时服药',
                    count: s.takeOk,
                    total: total,
                    color: Colors.green,
                    icon: Icons.check_circle_outline,
                  ),
                  const SizedBox(height: 12),
                  _StatCard(
                    label: '取药未服',
                    count: s.takeAndReturn,
                    total: total,
                    color: Colors.orange,
                    icon: Icons.undo,
                  ),
                  const SizedBox(height: 12),
                  _StatCard(
                    label: '开盖未取',
                    count: s.openNoTake,
                    total: total,
                    color: Colors.red.shade700,
                    icon: Icons.folder_open,
                  ),
                  const SizedBox(height: 12),
                  _StatCard(
                    label: '漏服',
                    count: s.missed,
                    total: total,
                    color: Colors.deepOrange,
                    icon: Icons.cancel_outlined,
                  ),
                  const SizedBox(height: 12),
                  _StatCard(
                    label: '其他',
                    count: s.other,
                    total: total,
                    color: Colors.grey,
                    icon: Icons.info_outline,
                  ),
                  const SizedBox(height: 20),
                  if (total > 0)
                    Card(
                      child: Padding(
                        padding: const EdgeInsets.all(16),
                        child: Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            const Text('服药情况', style: TextStyle(fontWeight: FontWeight.bold)),
                            const SizedBox(height: 12),
                            ClipRRect(
                              borderRadius: BorderRadius.circular(6),
                              child: LinearProgressIndicator(
                                value: total == 0 ? 0 : s.takeOk / total,
                                minHeight: 14,
                                backgroundColor: Colors.red.shade100,
                                valueColor: const AlwaysStoppedAnimation<Color>(Colors.green),
                              ),
                            ),
                            const SizedBox(height: 8),
                            Text(
                              '共 $total 条记录，按时服药 ${s.takeOk} 次',
                              style: const TextStyle(color: Colors.black54),
                            ),
                          ],
                        ),
                      ),
                    ),
                ],
              );
            },
          ),
        ],
      ),
    );
  }
}

class _StatCard extends StatelessWidget {
  const _StatCard({
    required this.label,
    required this.count,
    required this.total,
    required this.color,
    required this.icon,
  });

  final String label;
  final int count;
  final int total;
  final Color color;
  final IconData icon;

  @override
  Widget build(BuildContext context) {
    final pct = total == 0 ? 0.0 : count / total;
    return Card(
      child: Padding(
        padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 14),
        child: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(12),
              decoration: BoxDecoration(
                color: color.withValues(alpha: 0.1),
                borderRadius: BorderRadius.circular(12),
              ),
              child: Icon(icon, color: color, size: 28),
            ),
            const SizedBox(width: 16),
            Expanded(
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  Text(label, style: const TextStyle(fontSize: 15, fontWeight: FontWeight.w500)),
                  const SizedBox(height: 4),
                  ClipRRect(
                    borderRadius: BorderRadius.circular(4),
                    child: LinearProgressIndicator(
                      value: pct,
                      minHeight: 8,
                      backgroundColor: Colors.grey.shade200,
                      valueColor: AlwaysStoppedAnimation<Color>(color),
                    ),
                  ),
                ],
              ),
            ),
            const SizedBox(width: 16),
            Text(
              '$count',
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold, color: color),
            ),
          ],
        ),
      ),
    );
  }
}

class _RecordsListTab extends StatelessWidget {
  const _RecordsListTab({
    required this.loadingMore,
    required this.onLoadMore,
    required this.onRefresh,
  });

  final bool loadingMore;
  final Future<void> Function() onLoadMore;
  final Future<void> Function() onRefresh;

  @override
  Widget build(BuildContext context) {
    return Consumer<PillboxProvider>(
      builder: (context, provider, child) {
        if (provider.records.isEmpty) {
          return RefreshIndicator(
            onRefresh: onRefresh,
            child: ListView(
              physics: const AlwaysScrollableScrollPhysics(),
              padding: const EdgeInsets.all(16),
              children: const [
                SizedBox(height: 120),
                Center(
                  child: Text('暂无服药记录', textAlign: TextAlign.center),
                ),
              ],
            ),
          );
        }

        return RefreshIndicator(
          onRefresh: onRefresh,
          child: ListView.builder(
            physics: const AlwaysScrollableScrollPhysics(),
            padding: const EdgeInsets.all(16),
            itemCount: provider.records.length + 1,
          itemBuilder: (context, index) {
            if (index == provider.records.length) {
              if (provider.records.length >= provider.recordsTotal) {
                return const Padding(
                  padding: EdgeInsets.all(16),
                  child: Center(child: Text('已加载全部')),
                );
              }
              return Padding(
                padding: const EdgeInsets.all(16),
                child: Center(
                  child: loadingMore
                      ? const CircularProgressIndicator()
                      : TextButton(
                          onPressed: onLoadMore,
                          child: const Text('加载更多'),
                        ),
                ),
              );
            }
            final record = provider.records[index];
            final timeRaw = record['time'] ?? record['date'] ?? record['actual_time'] ?? '';
            final timeTitle = _formatRecordTimeDisplay(timeRaw);
            final behavior = record['behavior'] as String?;
            final v = _rowVisualFor(record);

            return Card(
              margin: const EdgeInsets.only(bottom: 12),
              child: ListTile(
                contentPadding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
                leading: Container(
                  width: 48,
                  height: 48,
                  decoration: BoxDecoration(
                    color: v.color.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(12),
                  ),
                  child: Icon(
                    v.icon,
                    color: v.color,
                  ),
                ),
                title: Text(timeTitle, style: const TextStyle(fontWeight: FontWeight.bold)),
                subtitle: Padding(
                  padding: const EdgeInsets.only(top: 4),
                  child: Text(
                    '药仓: ${record['compartment'] ?? "—"}\n'
                    '行为: ${_behaviorLabel(behavior)}\n'
                    '${record['plan_time'] != null ? "计划: ${record['plan_time']}" : ""}',
                    style: const TextStyle(height: 1.4),
                  ),
                ),
                isThreeLine: true,
                trailing: Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 6),
                  decoration: BoxDecoration(
                    color: v.color.withValues(alpha: 0.1),
                    borderRadius: BorderRadius.circular(20),
                  ),
                  child: Text(
                    v.trailing,
                    style: TextStyle(
                      color: v.color,
                      fontSize: 12,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                ),
              ),
            );
          },
          ),
        );
      },
    );
  }
}
