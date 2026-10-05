// pages/data/data.js —— 数据页：连手表 -> 看步数/电量/固件版本/运行时间
const app = getApp();

Page({
  data: {
    connected: false,
    busy: false,
    version: '--',
    status: { steps: '0', battery: '--', firmware: '--', uptime: '--' },
    showLog: false,
    log: ''
  },

  onLoad() {
    this.ble = app.globalData.ble;
    this.ble.onLog = (s) => this.addLog(s);
    this.ble.onStatus = (s) => this.handleStatus(s);
  },

  addLog(s) {
    console.log('[数据]', s);          // 详细信息在开发者工具 Console 里看
    const t = new Date().toTimeString().slice(0, 8);
    this.setData({ log: `[${t}] ${s}\n` + this.data.log.slice(0, 1800) });
  },

  toggleLog() {
    this.setData({ showLog: !this.data.showLog });
  },

  // 解析 "fw=1.6;up=123s;step=0;bat=--"
  handleStatus(text) {
    const st = {};
    text.split(';').forEach((kv) => {
      const i = kv.indexOf('=');
      if (i > 0) st[kv.slice(0, i).trim()] = kv.slice(i + 1).trim();
    });
    this.setData({
      status: {
        steps: st.step || '0',
        battery: st.bat || '--',
        firmware: st.fw || '--',
        uptime: st.up || '--'
      }
    });
    this.addLog('状态: ' + text);
  },

  async connect() {
    if (this.data.busy) return;
    this.setData({ busy: true });
    const ble = this.ble;
    try {
      this.addLog('打开蓝牙适配器...');
      await ble.openAdapter();

      this.addLog('扫描手表（先点一下手表屏幕叫醒它）...');
      const dev = await ble.scan();
      this.addLog('找到: ' + (dev.name || dev.localName || dev.deviceId));

      this.addLog('连接中...');
      await ble.connect(dev.deviceId);
      this.addLog('已连接 ✓');

      const ver = await ble.readVersion();
      this.addLog('手表固件版本: ' + ver);
      app.globalData.watchVersion = ver;

      const st = await ble.readStatus();
      this.handleStatus(st);

      await ble.startNotify();
      this.addLog('已订阅通知（升级进度会实时显示）');

      this.setData({ connected: true, version: ver });
      wx.showToast({ title: '连接成功', icon: 'success' });
    } catch (e) {
      this.addLog('出错: ' + (e.errMsg || e.message || JSON.stringify(e)));
      wx.showToast({ title: '连接失败', icon: 'none' });
    }
    this.setData({ busy: false });
  },

  disconnect() {
    this.ble.disconnect();
    this.setData({ connected: false, version: '--' });
    this.addLog('已断开');
  }
});
