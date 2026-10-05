// app.js —— 小程序入口
//
// 这里做了三件事：
//   1) 小程序【自身】的版本更新提示（微信官方机制，自动检测新版 -> 弹窗问用户）
//   2) 提供全局的蓝牙连接对象（各页面共用一条连接）
//   3) 如果用云开发模式（utils/fw.js 的 MODE = 'cloud'），初始化云环境

const WatchBle = require('./utils/ble.js');
const fw = require('./utils/fw.js');

App({
  globalData: {
    ble: null,
    version: '',          // 小程序自身版本
    watchVersion: ''      // 最近一次读到的手表固件版本
  },

  onLaunch() {
    // 全局共用一个蓝牙封装
    const ble = new WatchBle();
    ble.onLog = (s) => console.log('[BLE]', s);
    this.globalData.ble = ble;

    // 只在云开发模式下初始化（http 模式不用）
    try {
      if (wx.cloud && fw.MODE === 'cloud') {
        wx.cloud.init({ env: fw.CLOUD_ENV_ID || undefined, traceUser: true });
        console.log('云开发已初始化');
      }
    } catch (e) {
      console.log('云开发未初始化（用 http 模式即可）:', e);
    }

    try {
      this.globalData.version = wx.getAccountInfoSync().miniProgram.version || '开发版';
    } catch (e) {
      this.globalData.version = '开发版';
    }

    this.checkSelfUpdate();
  },

  // ---- 小程序自身更新（"打开时提示有新版本，是否更新"）----
  checkSelfUpdate() {
    if (!wx.getUpdateManager) return;
    const um = wx.getUpdateManager();

    um.onCheckForUpdate((res) => {
      console.log('是否有新版本:', res.hasUpdate);
    });

    um.onUpdateReady(() => {
      wx.showModal({
        title: '发现新版本',
        content: '新版本已经下载好，是否重启应用？',
        confirmText: '立即更新',
        cancelText: '稍后',
        success: (res) => {
          if (res.confirm) {
            um.applyUpdate();      // 应用新版本并重启
          }
        }
      });
    });

    um.onUpdateFailed(() => {
      wx.showModal({
        title: '更新失败',
        content: '新版本下载失败，可删除小程序后重新进入',
        showCancel: false
      });
    });
  }
});
