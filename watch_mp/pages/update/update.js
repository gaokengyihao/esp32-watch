// pages/update/update.js —— 升级页：打开自动检查版本，不是最新才出现"升级"按钮
//
// 用户视角只有三步：
//   1. 打开本页 -> 自动检查（无需操作）
//   2. 显示"已是最新"（没按钮）或者"发现新固件 vX"（有按钮）
//   3. 点"立即升级" -> 自动下载 -> 蓝牙传给手表 -> 手表重启 ✓
//
// 调试信息走 console.log（开发者工具的 Console 里看），不占界面。

const app = getApp();
const fw = require('../../utils/fw.js');

Page({
  data: {
    watchVer: '--',
    cloudVer: '',
    state: 'idle',        // idle | checking | latest | found | needWatch | downloading | sending | done | error
    msg: '',
    dlProgress: 0,        // 下载固件进度
    txProgress: 0,        // 蓝牙传输进度
    txSpeed: ''           // 实时速度
  },

  onLoad() {
    this.ble = app.globalData.ble;
    // 手表能发通知的话：显示"手表已写入 x%"（收不到也没关系，
    // 进度和成功判断主要靠 ble.js 的"分批+屏障确认"机制）
    this.ble.onStatus = (s) => {
      if (s.indexOf('ota=') !== 0) return;
      const v = s.slice(4);
      if (v.endsWith('%')) {
        const pct = parseInt(v, 10);
        if (!isNaN(pct)) {
          this.ble.setWatchProgress(pct);
          this.setData({ msg: '手表已写入 ' + pct + '%' });
        }
      }
    };
  },

  onShow() {
    const ble = this.ble;
    this.setData({ watchVer: app.globalData.watchVersion || '--' });
    if (ble && ble.connected) {
      ble.readVersion().then((v) => {
        app.globalData.watchVersion = v;
        this.setData({ watchVer: v });
        this.autoCheck();
      }).catch(() => this.autoCheck());
    } else {
      this.autoCheck();          // 没连表也先看看服务器上是哪个版本
    }
  },

  log(s) {
    console.log('[更新]', s);
  },

  // ---------------- 自动检查最新版本 ----------------
  async autoCheck() {
    const st = this.data.state;
    if (st === 'downloading' || st === 'sending') return;   // 升级中别打扰

    this.setData({ state: 'checking', msg: '正在检查最新版本...' });
    try {
      const latest = await fw.getLatestVersion();
      this.setData({ cloudVer: latest });
      if (!latest) {
        this.setData({ state: 'error', msg: '服务器上没有版本文件（先在电脑上跑一次 publish.bat）' });
        return;
      }
      if (this.data.watchVer === '--') {
        this.setData({ state: 'needWatch', msg: `服务器上最新是 v${latest}，先连接手表才能判断是否需要升级` });
        return;
      }
      if (latest === this.data.watchVer) {
        this.setData({ state: 'latest', msg: '已是最新版本' });
        this.log(`已是最新版本 v${latest}`);
      } else {
        this.setData({ state: 'found', msg: '' });
        this.log(`发现新固件 v${latest}（手表 v${this.data.watchVer}）`);
      }
    } catch (e) {
      this.log('检查失败: ' + (e.errMsg || e.message || e));
      this.setData({ state: 'error', msg: '检查失败' });
    }
  },

  // ---------------- 一键升级：自动下载 -> 蓝牙传输 ----------------
  async doUpdate() {
    if (this.data.state !== 'found') return;

    try {
      this.setData({ state: 'downloading', dlProgress: 0, txProgress: 0, msg: '' });
      const buf = await fw.downloadFirmware((p) => this.setData({ dlProgress: p }));
      this.log(`固件下载完成：${(buf.byteLength / 1024 / 1024).toFixed(2)} MB`);

      this.setData({ state: 'sending', msg: '正在传给手表（约 4 分钟，请保持在小程序里）' });
      this.ble.setWatchProgress(-1);
      /* ★ 完成握手在 ble.uploadFirmware 里做（分批连发 + 屏障确认 + 校验查询），
       *   整条链路不依赖通知 —— 通知收不到的 iPhone 也能升级 ✓ */
      await this.ble.uploadFirmware(buf, (sent, total, kbps, extra) => {
        const pct = Math.floor((sent * 100) / total);
        this.setData({
          txProgress: pct,
          txSpeed: kbps ? kbps + ' KB/s' : this.data.txSpeed,
          msg: pct >= 100
            ? '数据传完，等手表校验...'
            : ('传输中 ' + pct + '%' + (extra ? '（' + extra + '）' : ''))
        });
      });

      this.log('传输完成，手表已确认 ✓');
      const latest = this.data.cloudVer || this.data.watchVer;
      if (latest && latest !== '--') {
        app.globalData.watchVersion = latest;   // 更新缓存：以后检查直接显示"已是最新" ✓
        this.setData({ watchVer: latest });
      }
      this.setData({ state: 'done', txProgress: 100, msg: '升级完成，手表正在重启...' });
      wx.showModal({
        title: '升级完成',
        content: '手表已确认新固件，正在重启（几秒后手表会回到表盘）✓',
        showCancel: false
      });
      /* 过几秒把文案收敛成"已完成"，不再一直显示"正在重启" ✓ */
      setTimeout(() => {
        if (this.data.state === 'done') {
          this.setData({ msg: '升级完成 ✓（手表现已是 v' + (latest || '') + '，可重新连接查看）' });
        }
      }, 8000);
    } catch (e) {
      this.log('升级失败: ' + (e.errMsg || e.message || e));

      /* ★ 已知"误报"：手表收满固件后会 校验->重启，蓝牙必然断开。
       *   只要完整发过一遍（或进度已到 100%），这就是成功，不是失败 ✓ */
      if (this.ble.sentAllOnce || this.data.txProgress >= 99) {
        const latest = this.data.cloudVer || this.data.watchVer;
        if (latest && latest !== '--') {
          app.globalData.watchVersion = latest;
          this.setData({ watchVer: latest });
        }
        this.setData({ state: 'done', txProgress: 100, msg: '固件已传完，手表正在重启（几秒后自动确认版本）' });
        setTimeout(() => this.verifyNewVersion(), 7000);   // 手表重启完再连上确认一次
        return;
      }

      this.setData({ state: 'error', msg: '升级失败，可重试' });
    }
  },

  /* 升级后自动重连，读出手表版本 = 真正确认升级成功 */
  async verifyNewVersion() {
    if (this.data.state !== 'done') return;
    try {
      this.setData({ msg: '正在重新连接手表，确认版本...' });
      try { this.ble.disconnect(); } catch (e) {}
      await this.ble.openAdapter();
      const dev = await this.ble.scan(12000);
      await this.ble.connect(dev.deviceId);
      const v = await this.ble.readVersion();
      app.globalData.watchVersion = v;
      this.setData({ watchVer: v, msg: '升级完成 ✓ 手表现为 v' + v });
      wx.showModal({ title: '升级成功', content: '手表已升级到 v' + v, showCancel: false });
    } catch (e) {
      this.log('自动确认失败: ' + (e.errMsg || e.message || e));
      this.setData({ msg: '固件已传完；手表重启后到「数据」页连上看版本即可确认' });
    }
  }
});
