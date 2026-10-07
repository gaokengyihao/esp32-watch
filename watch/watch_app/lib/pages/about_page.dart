import 'package:flutter/material.dart';

/// 关于页：项目介绍 + 蓝牙服务说明（对接固件用）
class AboutPage extends StatelessWidget {
  const AboutPage({super.key});

  @override
  Widget build(BuildContext context) {
    return ListView(
      padding: const EdgeInsets.all(12),
      children: const [
        Card(
          child: Padding(
            padding: EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('ESP32-S3 智能手表',
                    style:
                        TextStyle(fontSize: 20, fontWeight: FontWeight.bold)),
                SizedBox(height: 8),
                Text('屏幕 + LVGL v9 + 触摸手势 + WiFi 对时 + 低功耗 + '
                    'A/B 双分区 OTA + 蓝牙（BLE）'),
              ],
            ),
          ),
        ),
        Card(
          child: Padding(
            padding: EdgeInsets.all(16),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                Text('蓝牙服务（固件端 main/ble_watch.c）',
                    style: TextStyle(fontWeight: FontWeight.bold)),
                SizedBox(height: 8),
                Text(
                  '设备名: ESP32-Watch\n\n'
                  '0xFF00  服务\n'
                  '0xFF01  读       固件版本\n'
                  '0xFF02  读+通知   状态数据\n'
                  '0xFF03  写       命令\n'
                  '0xFF04  写       固件数据（OTA）',
                  style: TextStyle(fontSize: 13, height: 1.6),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }
}
