import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../constants.dart';
import '../providers/pillbox_provider.dart';

class SettingsPage extends StatefulWidget {
  const SettingsPage({super.key});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  final _ipController = TextEditingController();

  @override
  void initState() {
    super.initState();
    _ipController.text = AppConstants.defaultDeviceHost;
    WidgetsBinding.instance.addPostFrameCallback((_) => _loadSavedIp());
  }

  Future<void> _loadSavedIp() async {
    if (!mounted) return;
    final p = context.read<PillboxProvider>();
    final uri = p.baseUrl.replaceFirst('http://', '').replaceFirst('https://', '');
    if (uri.isNotEmpty && AppConstants.isValidIpv4(uri)) {
      setState(() => _ipController.text = uri);
    }
  }

  @override
  void dispose() {
    _ipController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('设置', style: TextStyle(fontWeight: FontWeight.bold)),
      ),
      body: ListView(
        padding: const EdgeInsets.all(16),
        children: [
          const SizedBox(height: 20),
          Center(
            child: Container(
              padding: const EdgeInsets.all(24),
              decoration: BoxDecoration(
                color: const Color(0xFF00B4D8).withValues(alpha: 0.1),
                shape: BoxShape.circle,
              ),
              child: const Icon(
                Icons.medical_services_outlined,
                size: 64,
                color: Color(0xFF00B4D8),
              ),
            ),
          ),
          const SizedBox(height: 32),
          Card(
            child: Padding(
              padding: const EdgeInsets.all(20),
              child: Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Row(
                    children: [
                      Icon(Icons.router, color: Colors.grey),
                      SizedBox(width: 8),
                      Text(
                        '设备连接',
                        style: TextStyle(fontSize: 16, fontWeight: FontWeight.bold),
                      ),
                    ],
                  ),
                  const SizedBox(height: 20),
                  TextField(
                    controller: _ipController,
                    decoration: InputDecoration(
                      labelText: '药盒 IP 地址',
                      hintText: '如：设备实际分配的 IPv4 地址（配网 AP：192.168.4.1）',
                      prefixIcon: const Icon(Icons.wifi),
                      filled: true,
                      fillColor: Colors.grey.shade50,
                      border: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: BorderSide(color: Colors.grey.shade300),
                      ),
                      enabledBorder: OutlineInputBorder(
                        borderRadius: BorderRadius.circular(12),
                        borderSide: BorderSide(color: Colors.grey.shade300),
                      ),
                    ),
                  ),
                  const SizedBox(height: 24),
                  SizedBox(
                    width: double.infinity,
                    child: ElevatedButton.icon(
                      onPressed: () async {
                        final host = _ipController.text.trim();
                        if (host.isEmpty) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('请输入药盒 IP 地址')),
                          );
                          return;
                        }
                        if (!AppConstants.isValidIpv4(host)) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            const SnackBar(content: Text('IP 格式不正确，请输入设备实际分配的 IPv4 地址')),
                          );
                          return;
                        }
                        final url = 'http://$host';
                        final prov = context.read<PillboxProvider>();
                        prov.setBaseUrl(url);
                        await prov.saveDeviceHost(host);
                        await prov.checkStatus(trackLoading: true);
                        if (!context.mounted) return;
                        final p = context.read<PillboxProvider>();
                        if (p.isConnected) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(content: Text('本次状态请求成功：$url（已记住 IP）')),
                          );
                        } else if (p.lastError != null) {
                          ScaffoldMessenger.of(context).showSnackBar(
                            SnackBar(content: Text('连接失败: ${p.lastError}')),
                          );
                        }
                      },
                      icon: const Icon(Icons.link, color: Colors.white),
                      label: const Text('保存并连接', style: TextStyle(color: Colors.white, fontSize: 16)),
                      style: ElevatedButton.styleFrom(
                        backgroundColor: const Color(0xFF00B4D8),
                        padding: const EdgeInsets.symmetric(vertical: 14),
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
        ],
      ),
    );
  }

}
