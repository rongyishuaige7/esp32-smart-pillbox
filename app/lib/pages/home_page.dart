import 'dart:async';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/pillbox_provider.dart';

class HomePage extends StatefulWidget {
  const HomePage({super.key});

  @override
  State<HomePage> createState() => _HomePageState();
}

class _HomePageState extends State<HomePage> {
  Timer? _pollTimer;
  late final AppLifecycleListener _lifecycle;

  void _startPoll() {
    _pollTimer?.cancel();
    _pollTimer = Timer.periodic(const Duration(seconds: 5), (_) {
      if (!mounted) return;
      context.read<PillboxProvider>().checkStatus();
    });
  }

  @override
  void initState() {
    super.initState();
    _lifecycle = AppLifecycleListener(
      onStateChange: (AppLifecycleState state) {
        if (!mounted) return;
        if (state == AppLifecycleState.resumed) {
          context.read<PillboxProvider>().refreshAll(trackLoading: false);
          _startPoll();
        } else if (state == AppLifecycleState.paused ||
            state == AppLifecycleState.inactive ||
            state == AppLifecycleState.hidden) {
          _pollTimer?.cancel();
        }
      },
    );
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final p = context.read<PillboxProvider>();
      p.refreshAll();
      _startPoll();
    });
  }

  @override
  void dispose() {
    _lifecycle.dispose();
    _pollTimer?.cancel();
    super.dispose();
  }

  void _showAddPlanDialog(BuildContext context) {
    final nameController = TextEditingController();
    int compartment = 1;
    final times = <TimeOfDay>[];

    showDialog(
      context: context,
      builder: (dialogContext) {
        return StatefulBuilder(
          builder: (context, setState) {
            final timesStr = times
                .map((t) => '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}')
                .join(',');
            return AlertDialog(
              title: const Text('添加用药计划'),
              content: SingleChildScrollView(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    TextField(
                      controller: nameController,
                      decoration: const InputDecoration(
                        labelText: '药物名称',
                        hintText: '如：感冒药',
                      ),
                    ),
                    const SizedBox(height: 16),
                    Row(
                      children: [
                        const Text('服药时间：'),
                        const SizedBox(width: 8),
                        Expanded(child: Text(timesStr.isEmpty ? '未设置' : timesStr)),
                        TextButton.icon(
                          icon: const Icon(Icons.add),
                          label: const Text('添加时间'),
                          onPressed: () async {
                            final picked = await showTimePicker(
                              context: context,
                              initialTime: TimeOfDay.now(),
                            );
                            if (picked != null) {
                              setState(() => times.add(picked));
                            }
                          },
                        ),
                      ],
                    ),
                    if (times.isNotEmpty)
                      Wrap(
                        spacing: 4,
                        children: times.asMap().entries.map((e) {
                          final t = e.value;
                          final s =
                              '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}';
                          return Chip(
                            label: Text(s),
                            onDeleted: () => setState(() => times.removeAt(e.key)),
                          );
                        }).toList(),
                      ),
                    const SizedBox(height: 16),
                    // Controlled field in dialog; `value` is still required for updates.
                    DropdownButtonFormField<int>(
                      // ignore: deprecated_member_use
                      value: compartment,
                      decoration: const InputDecoration(
                        labelText: '药仓',
                      ),
                      items: const [
                        DropdownMenuItem(value: 1, child: Text('药仓1')),
                        DropdownMenuItem(value: 2, child: Text('药仓2')),
                      ],
                      onChanged: (value) {
                        setState(() => compartment = value ?? 1);
                      },
                    ),
                  ],
                ),
              ),
              actions: [
                TextButton(
                  onPressed: () => Navigator.pop(dialogContext),
                  child: const Text('取消'),
                ),
                ElevatedButton(
                  onPressed: () async {
                    if (nameController.text.isEmpty) {
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(content: Text('请填写药物名称')),
                      );
                      return;
                    }
                    if (times.isEmpty) {
                      ScaffoldMessenger.of(context).showSnackBar(
                        const SnackBar(content: Text('请至少添加一个服药时间')),
                      );
                      return;
                    }
                    final timesStrSubmit = times
                        .map((t) =>
                            '${t.hour.toString().padLeft(2, '0')}:${t.minute.toString().padLeft(2, '0')}')
                        .join(',');
                    final newPlan = {
                      'id': DateTime.now().millisecondsSinceEpoch,
                      'name': nameController.text,
                      'compartment': compartment,
                      'times': timesStrSubmit,
                      'enabled': true,
                    };
                    final ok = await context.read<PillboxProvider>().addPlan(newPlan);
                    if (!context.mounted) return;
                    Navigator.pop(dialogContext);
                    if (ok) {
                      ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('已添加')));
                    } else if (context.read<PillboxProvider>().lastError != null) {
                      ScaffoldMessenger.of(context).showSnackBar(
                        SnackBar(content: Text('添加失败: ${context.read<PillboxProvider>().lastError}')),
                      );
                    }
                  },
                  child: const Text('添加'),
                ),
              ],
            );
          },
        );
      },
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('智能药盒', style: TextStyle(fontWeight: FontWeight.bold)),
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
                onPressed: provider.isLoading ? null : () => provider.refreshAll(),
              );
            },
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () => _showAddPlanDialog(context),
        child: const Icon(Icons.add),
      ),
      body: Consumer<PillboxProvider>(
        builder: (context, provider, child) {
          return RefreshIndicator(
            onRefresh: () => provider.refreshAll(),
            child: ListView(
              padding: const EdgeInsets.all(16),
              children: [
                if (provider.missedPending1 || provider.missedPending2)
                  Card(
                    color: Colors.orange.shade100,
                    child: ListTile(
                      leading: const Icon(Icons.warning_amber_rounded, color: Colors.deepOrange),
                      title: const Text('漏服监测中', style: TextStyle(fontWeight: FontWeight.bold)),
                      subtitle: Text(
                        '药仓1: ${provider.missedPending1 ? "等待服药确认" : "正常"}  '
                        '药仓2: ${provider.missedPending2 ? "等待服药确认" : "正常"}',
                      ),
                    ),
                  ),
                if (provider.missedPending1 || provider.missedPending2) const SizedBox(height: 8),
                if (provider.lastError != null)
                  Card(
                    color: Colors.red.shade100,
                    child: ListTile(
                      title: const Text('连接错误', style: TextStyle(fontWeight: FontWeight.bold)),
                      subtitle: Text(provider.lastError!),
                      trailing: IconButton(
                        icon: const Icon(Icons.close),
                        onPressed: () => provider.clearError(),
                      ),
                    ),
                  ),
                if (provider.lastError != null) const SizedBox(height: 8),
                Container(
                  decoration: BoxDecoration(
                    gradient: LinearGradient(
                      colors: provider.isConnected
                          ? [const Color(0xFF00B4D8), const Color(0xFF0077B6)]
                          : [Colors.grey.shade400, Colors.grey.shade600],
                      begin: Alignment.topLeft,
                      end: Alignment.bottomRight,
                    ),
                    borderRadius: BorderRadius.circular(20),
                    boxShadow: [
                      BoxShadow(
                        color: (provider.isConnected ? const Color(0xFF00B4D8) : Colors.grey).withValues(alpha: 0.3),
                        blurRadius: 10,
                        offset: const Offset(0, 5),
                      ),
                    ],
                  ),
                  child: Padding(
                    padding: const EdgeInsets.all(20),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Container(
                              padding: const EdgeInsets.all(12),
                              decoration: BoxDecoration(
                                color: Colors.white.withValues(alpha: 0.2),
                                shape: BoxShape.circle,
                              ),
                              child: Icon(
                                provider.isConnected ? Icons.wifi : Icons.wifi_off,
                                color: Colors.white,
                                size: 32,
                              ),
                            ),
                            const SizedBox(width: 16),
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text(
                                    provider.isConnected ? '本次状态请求成功' : '尚未连接设备',
                                    style: const TextStyle(
                                      fontSize: 22,
                                      fontWeight: FontWeight.bold,
                                      color: Colors.white,
                                    ),
                                  ),
                                  const SizedBox(height: 4),
                                  Text(
                                    provider.ipAddress.isNotEmpty ? 'IP: ${provider.ipAddress}' : '请在设置中连接设备',
                                    style: const TextStyle(color: Colors.white70, fontSize: 14),
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ),
                        if (provider.isConnected) ...[
                          const SizedBox(height: 20),
                          Container(height: 1, color: Colors.white.withValues(alpha: 0.2)),
                          const SizedBox(height: 16),
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceAround,
                            children: [
                              _buildStatusItem(Icons.timer_outlined, '运行时间', _formatUptime(provider.uptime)),
                              _buildStatusItem(Icons.sensors, '药仓1', provider.sensor1Motion ? '拿取中' : '静置'),
                              _buildStatusItem(Icons.sensors, '药仓2', provider.sensor2Motion ? '拿取中' : '静置'),
                            ],
                          ),
                        ],
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 16),
                const SizedBox(height: 16),
                const Padding(
                  padding: EdgeInsets.only(left: 8, bottom: 8),
                  child: Text(
                    '快捷操作',
                    style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                  ),
                ),
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(12),
                    child: Row(
                      children: [
                        Expanded(
                          child: ElevatedButton.icon(
                            onPressed: () async {
                              final ok = await provider.triggerReminder();
                              if (!context.mounted) return;
                              if (ok) {
                                ScaffoldMessenger.of(context).showSnackBar(
                                  const SnackBar(content: Text('提醒已触发')),
                                );
                              } else if (provider.lastError != null) {
                                ScaffoldMessenger.of(context).showSnackBar(
                                  SnackBar(content: Text('触发失败: ${provider.lastError}')),
                                );
                              }
                            },
                            icon: const Icon(Icons.notifications_active, color: Colors.white),
                            label: const Text('测试药盒提醒', style: TextStyle(color: Colors.white, fontSize: 16)),
                            style: ElevatedButton.styleFrom(
                              backgroundColor: Colors.orange,
                              padding: const EdgeInsets.symmetric(vertical: 12),
                              shape: RoundedRectangleBorder(
                                borderRadius: BorderRadius.circular(12),
                              ),
                            ),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                const SizedBox(height: 16),
                const Padding(
                  padding: EdgeInsets.only(left: 8, bottom: 8),
                  child: Text(
                    '当前用药计划',
                    style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                  ),
                ),
                if (provider.plans.isEmpty)
                  const Card(
                    child: Padding(
                      padding: EdgeInsets.all(32),
                      child: Center(
                        child: Column(
                          children: [
                            Icon(Icons.calendar_today, size: 48, color: Colors.grey),
                            SizedBox(height: 16),
                            Text('暂无用药计划', style: TextStyle(color: Colors.grey, fontSize: 16)),
                          ],
                        ),
                      ),
                    ),
                  )
                else
                  ...List.generate(provider.plans.length, (index) {
                    final plan = provider.plans[index];
                    final isEnabled = plan['enabled'] == true;
                    return Card(
                      color: isEnabled ? Colors.white : Colors.grey.shade100,
                      child: Padding(
                        padding: const EdgeInsets.symmetric(vertical: 8),
                        child: ListTile(
                          leading: Container(
                            width: 48,
                            height: 48,
                            decoration: BoxDecoration(
                              color: isEnabled ? const Color(0xFF00B4D8).withValues(alpha: 0.1) : Colors.grey.shade300,
                              borderRadius: BorderRadius.circular(12),
                            ),
                            child: Center(
                              child: Icon(
                                Icons.medication,
                                color: isEnabled ? const Color(0xFF00B4D8) : Colors.grey,
                              ),
                            ),
                          ),
                          title: Text(
                            plan['name'] ?? '未知',
                            style: TextStyle(
                              fontWeight: FontWeight.bold,
                              color: isEnabled ? Colors.black87 : Colors.grey,
                            ),
                          ),
                          subtitle: Padding(
                            padding: const EdgeInsets.only(top: 4),
                            child: Text(
                              '时间: ${plan['times'] ?? "未设置"}\n药仓: ${plan['compartment'] ?? 1}',
                              style: TextStyle(height: 1.4, color: isEnabled ? Colors.black54 : Colors.grey),
                            ),
                          ),
                          isThreeLine: true,
                          trailing: Column(
                            mainAxisAlignment: MainAxisAlignment.center,
                            children: [
                              Switch(
                                value: isEnabled,
                                activeThumbColor: const Color(0xFF00B4D8),
                                onChanged: (value) => provider.updatePlanEnabled(index, value),
                              ),
                            ],
                          ),
                          onLongPress: () async {
                            final confirm = await showDialog<bool>(
                              context: context,
                              builder: (c) => AlertDialog(
                                title: const Text('删除计划'),
                                content: const Text('确定要删除这个用药计划吗？'),
                                actions: [
                                  TextButton(onPressed: () => Navigator.pop(c, false), child: const Text('取消')),
                                  TextButton(
                                    onPressed: () => Navigator.pop(c, true),
                                    child: const Text('删除', style: TextStyle(color: Colors.red)),
                                  ),
                                ],
                              ),
                            );
                            if (confirm == true) {
                              final ok = await provider.deletePlan(index);
                              if (ok && context.mounted) {
                                ScaffoldMessenger.of(context).showSnackBar(const SnackBar(content: Text('已删除')));
                              }
                            }
                          },
                        ),
                      ),
                    );
                  }),
              ],
            ),
          );
        },
      ),
    );
  }

  Widget _buildStatusItem(IconData icon, String label, String value) {
    return Column(
      children: [
        Icon(icon, color: Colors.white70, size: 24),
        const SizedBox(height: 6),
        Text(label, style: const TextStyle(color: Colors.white70, fontSize: 12)),
        const SizedBox(height: 2),
        Text(value, style: const TextStyle(color: Colors.white, fontWeight: FontWeight.bold, fontSize: 14)),
      ],
    );
  }

  String _formatUptime(int milliseconds) {
    final seconds = milliseconds ~/ 1000;
    final minutes = seconds ~/ 60;
    final hours = minutes ~/ 60;
    final days = hours ~/ 24;

    if (days > 0) {
      return '$days天 ${hours % 24}小时';
    } else if (hours > 0) {
      return '$hours小时 ${minutes % 60}分钟';
    } else if (minutes > 0) {
      return '$minutes分钟';
    } else {
      return '$seconds秒';
    }
  }
}
