import 'package:flutter/material.dart';

import 'ble_service.dart';
import 'pages/about_page.dart';
import 'pages/home_page.dart';
import 'pages/update_page.dart';

void main() => runApp(const WatchApp());

class WatchApp extends StatelessWidget {
  const WatchApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'ESP32 手表',
      theme: ThemeData(
        colorSchemeSeed: const Color(0xFF00C8FF),
        useMaterial3: true,
        brightness: Brightness.dark,
      ),
      home: const RootPage(),
    );
  }
}

class RootPage extends StatefulWidget {
  const RootPage({super.key});
  @override
  State<RootPage> createState() => _RootPageState();
}

class _RootPageState extends State<RootPage> {
  int _index = 0;

  @override
  Widget build(BuildContext context) {
    return ListenableBuilder(
      listenable: ble,
      builder: (context, _) {
        return Scaffold(
          appBar: AppBar(
            title: Text(['手表数据', '蓝牙升级', '关于'][_index]),
            actions: [
              // 模拟模式开关：没蓝牙/没手表时也能调界面
              Row(
                children: [
                  const Text('模拟', style: TextStyle(fontSize: 12)),
                  Switch(
                    value: ble.mockMode,
                    onChanged: (v) => ble.setMockMode(v),
                  ),
                ],
              ),
            ],
          ),
          body: [const HomePage(), const UpdatePage(), const AboutPage()][_index],
          bottomNavigationBar: NavigationBar(
            selectedIndex: _index,
            onDestinationSelected: (i) => setState(() => _index = i),
            destinations: const [
              NavigationDestination(icon: Icon(Icons.watch), label: '数据'),
              NavigationDestination(icon: Icon(Icons.system_update), label: '升级'),
              NavigationDestination(icon: Icon(Icons.info_outline), label: '关于'),
            ],
          ),
        );
      },
    );
  }
}
