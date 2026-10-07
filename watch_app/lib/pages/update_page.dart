import 'package:flutter/material.dart';

import '../ble_service.dart';

/// 升级页：显示当前版本 -> 选固件文件 -> 蓝牙传给手表（OTA over BLE）
class UpdatePage extends StatefulWidget {
  const UpdatePage({super.key});

  @override
  State<UpdatePage> createState() => _UpdatePageState();
}

class _UpdatePageState extends State<UpdatePage> {
  late final TextEditingController _pathCtrl =
      TextEditingController(text: ble.firmwarePath);

  @override
  void dispose() {
    _pathCtrl.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        Card(
          child: ListTile(
            leading: const Icon(Icons.memory),
            title: const Text('手表版本'),
            trailing: Text(ble.status.version,
                style:
                    const TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
          ),
        ),
        const SizedBox(height: 8),
        TextField(
          controller: _pathCtrl,
          decoration: const InputDecoration(
            labelText: '固件文件路径',
            helperText: '留默认即可（idf.py build 的产物）',
            border: OutlineInputBorder(),
          ),
          onChanged: (v) => ble.firmwarePath = v,
        ),
        const SizedBox(height: 16),
        FilledButton.icon(
          onPressed: ble.busy ? null : ble.uploadFirmware,
          icon: const Icon(Icons.upload),
          label: const Text('开始升级（蓝牙传输）'),
        ),
        const SizedBox(height: 16),
        LinearProgressIndicator(value: ble.otaProgress),
        const SizedBox(height: 8),
        Text(ble.otaMessage.isEmpty ? '等待开始' : ble.otaMessage,
            textAlign: TextAlign.center),
        const SizedBox(height: 24),
        const Card(
          child: Padding(
            padding: EdgeInsets.all(12),
            child: Text(
              '蓝牙 OTA 说明\n\n'
              '· 固件先写入手表的"另一块"分区，传一半断了也不影响使用\n'
              '· 传完手表自动校验+重启进新版本（A/B 分区 + 回滚保护）\n'
              '· 正式产品还可以加：固件签名校验、云端版本检查\n'
              '· 手机联网后自动检查新版本 -> 这就是"手机上更新手表"的完整链路',
              style: TextStyle(fontSize: 13, height: 1.6),
            ),
          ),
        ),
      ],
    );
  }
}
