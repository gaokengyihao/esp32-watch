import 'package:flutter/material.dart';

import '../ble_service.dart';

/// 数据页：步数 / 电量 / 版本 / 连接状态 + 连接按钮
class HomePage extends StatelessWidget {
  const HomePage({super.key});

  Widget _card(String title, String value, IconData icon, {String? sub}) {
    return Card(
      child: ListTile(
        leading: Icon(icon, size: 32),
        title: Text(title),
        subtitle: sub == null ? null : Text(sub),
        trailing: Text(
          value,
          style: const TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    final s = ble.status;
    return ListView(
      padding: const EdgeInsets.all(12),
      children: [
        _card('步数', s.steps, Icons.directions_walk,
            sub: '以后接 MPU6050 就是真实步数'),
        _card('电量', s.battery, Icons.battery_full,
            sub: '要加电池分压电路到 ADC（暂显示 --）'),
        _card('固件版本', s.version, Icons.memory, sub: '手表当前跑的版本'),
        _card('运行时间', s.uptime, Icons.timer_outlined),
        const SizedBox(height: 8),
        Row(
          children: [
            Expanded(
              child: FilledButton.icon(
                onPressed: ble.busy ? null : ble.connect,
                icon: const Icon(Icons.bluetooth),
                label: Text(ble.connected ? '已连接' : '连接手表'),
              ),
            ),
            const SizedBox(width: 8),
            OutlinedButton(
              onPressed: ble.connected ? ble.disconnect : null,
              child: const Text('断开'),
            ),
          ],
        ),
        const SizedBox(height: 16),
        Text('日志', style: Theme.of(context).textTheme.titleSmall),
        Container(
          height: 160,
          padding: const EdgeInsets.all(8),
          decoration: BoxDecoration(
            color: Colors.black26,
            borderRadius: BorderRadius.circular(8),
          ),
          child: SingleChildScrollView(
            child: Text(ble.logText, style: const TextStyle(fontSize: 12)),
          ),
        ),
      ],
    );
  }
}
