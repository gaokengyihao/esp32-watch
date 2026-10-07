/*
 * ble_service.dart —— 手表蓝牙封装（对应固件 main/ble_watch.c）
 *
 * 手表端的服务/特征值（16 位 UUID）：
 *   0xFF00  服务
 *   0xFF01  读        固件版本      -> "1.6"
 *   0xFF02  读 + 通知  状态数据      -> "fw=1.6;up=123s;step=0;bat=--"
 *   0xFF03  写        命令         -> "reboot"
 *   0xFF04  写        固件数据(OTA) -> 先写 4 字节总长度，再按块传 .bin
 *
 * ★ 模拟模式（mockMode = true）：
 *   电脑没有蓝牙适配器、或者手上没手表时，用假数据把界面跑起来，
 *   页面/交互照常能调。等蓝牙 dongle 到了，把开关一关就是真数据。
 */
import 'dart:async';
import 'dart:io';

import 'package:flutter/foundation.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

class WatchStatus {
  String version = '--';
  String uptime = '--';
  String steps = '0';
  String battery = '--';

  /// 解析 "fw=1.6;up=123s;step=0;bat=--"
  void parse(String raw) {
    for (final part in raw.split(';')) {
      final kv = part.split('=');
      if (kv.length != 2) continue;
      switch (kv[0].trim()) {
        case 'fw':
          version = kv[1];
          break;
        case 'up':
          uptime = kv[1];
          break;
        case 'step':
          steps = kv[1];
          break;
        case 'bat':
          battery = kv[1];
          break;
      }
    }
  }
}

/// 全局单例：整个 App 共用一个蓝牙连接
final BleService ble = BleService();

class BleService extends ChangeNotifier {
  static const String deviceName = 'ESP32-Watch';
  static final Guid svcWatch = Guid('0000ff00-0000-1000-8000-00805f9b34fb');
  static final Guid chrVersion = Guid('0000ff01-0000-1000-8000-00805f9b34fb');
  static final Guid chrStatus = Guid('0000ff02-0000-1000-8000-00805f9b34fb');
  static final Guid chrCmd = Guid('0000ff03-0000-1000-8000-00805f9b34fb');
  static final Guid chrOta = Guid('0000ff04-0000-1000-8000-00805f9b34fb');

  // ---- 状态（界面绑定这些） ----
  bool mockMode = true; // true = 假数据（先看界面）
  bool connected = false;
  bool busy = false;
  String logText = '';
  final WatchStatus status = WatchStatus();

  // ---- OTA ----
  double otaProgress = 0;
  String otaMessage = '';
  String firmwarePath = 'D:\\esp32\\esp32s3_watch\\build\\esp32s3_watch.bin';

  BluetoothDevice? _dev;
  BluetoothCharacteristic? _cVersion;
  BluetoothCharacteristic? _cStatus;
  BluetoothCharacteristic? _cOta;
  StreamSubscription? _statusSub;
  Timer? _mockTimer;

  BleService() {
    _startMockIfNeeded();
  }

  void _log(String s) {
    logText = '$s\n$logText';
    if (logText.length > 2000) logText = logText.substring(0, 2000);
    notifyListeners();
  }

  void setMockMode(bool v) {
    mockMode = v;
    if (v) {
      _startMockIfNeeded();
    } else {
      _mockTimer?.cancel();
      _mockTimer = null;
    }
    notifyListeners();
  }

  /// 模拟模式：每 2 秒编一个假状态，界面就有东西看了
  void _startMockIfNeeded() {
    if (!mockMode || _mockTimer != null) return;
    var t = 0;
    _mockTimer = Timer.periodic(const Duration(seconds: 2), (_) {
      t += 2;
      status.version = '1.6';
      status.uptime = '${t}s';
      status.steps = '${1200 + t * 3}';
      status.battery = '87%';
      notifyListeners();
    });
  }

  // ---------------------------------------------------------------- 扫描/连接
  Future<void> connect() async {
    if (mockMode) {
      connected = true;
      status.version = '1.6';
      _log('【模拟】已连接手表');
      notifyListeners();
      return;
    }

    busy = true;
    _log('扫描手表（先点一下手表屏幕叫醒它）...');
    notifyListeners();
    try {
      await FlutterBluePlus.startScan(timeout: const Duration(seconds: 15));
      BluetoothDevice? found;
      await for (final r in FlutterBluePlus.scanResults) {
        for (final d in r) {
          if (d.device.platformName == deviceName) {
            found = d.device;
            break;
          }
        }
        if (found != null) break;
      }
      await FlutterBluePlus.stopScan();
      if (found == null) {
        _log('没找到手表（叫醒它再试）');
        busy = false;
        notifyListeners();
        return;
      }

      _dev = found;
      _log('连接 ${found.platformName} ...');
      await found.connect();
      connected = true;

      final services = await found.discoverServices();
      for (final s in services) {
        if (s.uuid == svcWatch) {
          for (final c in s.characteristics) {
            if (c.uuid == chrVersion) _cVersion = c;
            if (c.uuid == chrStatus) _cStatus = c;
            if (c.uuid == chrOta) _cOta = c;
          }
        }
      }

      if (_cVersion != null) {
        final v = await _cVersion!.read();
        status.version = String.fromCharCodes(v);
        _log('手表版本：${status.version}');
      }
      if (_cStatus != null) {
        await _cStatus!.setNotifyValue(true);
        _statusSub = _cStatus!.onValueReceived.listen((v) {
          final s = String.fromCharCodes(v);
          status.parse(s);
          if (s.startsWith('ota=')) otaMessage = s;
          notifyListeners();
        });
        final v = await _cStatus!.read();
        status.parse(String.fromCharCodes(v));
      }
    } catch (e) {
      _log('连接出错：$e');
      connected = false;
    }
    busy = false;
    notifyListeners();
  }

  Future<void> disconnect() async {
    await _statusSub?.cancel();
    _statusSub = null;
    if (_dev != null && !mockMode) {
      try {
        await _dev!.disconnect();
      } catch (_) {}
    }
    _dev = null;
    connected = false;
    notifyListeners();
  }

  // ------------------------------------------------------------------- OTA
  /// 把本地固件文件通过蓝牙传给手表（协议见 ble_watch.c 头部注释）
  Future<void> uploadFirmware() async {
    otaProgress = 0;
    otaMessage = '准备中...';
    notifyListeners();

    final file = File(firmwarePath);
    if (!file.existsSync()) {
      otaMessage = '找不到固件文件：$firmwarePath';
      notifyListeners();
      return;
    }
    final bytes = await file.readAsBytes();
    _log('固件：${bytes.length} 字节');

    if (mockMode) {
      // 模拟模式：走一遍进度条，方便调界面
      otaMessage = '【模拟】传输中';
      for (var i = 0; i <= 100; i += 5) {
        otaProgress = i / 100;
        notifyListeners();
        await Future.delayed(const Duration(milliseconds: 150));
      }
      otaMessage = '【模拟】升级完成（手表重启）';
      notifyListeners();
      return;
    }

    if (_cOta == null) {
      otaMessage = '没连上手表（或没有 OTA 特征值）';
      notifyListeners();
      return;
    }

    try {
      // 1) 先写 4 字节小端总长度
      final header = Uint8List(4);
      ByteData.view(header.buffer).setUint32(0, bytes.length, Endian.little);
      await _cOta!.write(header.toList());
      otaMessage = '传输中...';

      // 2) 按 244 字节一块传（MTU 512 时的安全值）
      const chunk = 244;
      for (var i = 0; i < bytes.length; i += chunk) {
        final end = (i + chunk < bytes.length) ? i + chunk : bytes.length;
        await _cOta!.write(bytes.sublist(i, end), withoutResponse: true);
        otaProgress = end / bytes.length;
        notifyListeners();
      }
      otaMessage = '传完，等手表校验+重启...';
      _log('固件传完 ✓');
    } catch (e) {
      otaMessage = '传输失败：$e';
      _log(otaMessage);
    }
    notifyListeners();
  }

  @override
  void dispose() {
    _statusSub?.cancel();
    _mockTimer?.cancel();
    super.dispose();
  }
}
