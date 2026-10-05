// pages/about/about.js —— 关于页
const app = getApp();

Page({
  data: {
    version: '开发版'
  },

  onLoad() {
    this.setData({ version: app.globalData.version || '开发版' });
  },

  // 手动检查小程序自身更新（微信每次启动都会自动检查，这里是给用户一个按钮）
  checkUpdate() {
    wx.showModal({
      title: '关于小程序更新',
      content: '微信会在每次打开时自动检查新版本，有新版会弹窗提示你重启更新 ✓\n\n'
        + '（开发阶段：在开发者工具上传新版本并在后台"提交审核/发布"后，'
        + '手机端通常 24 小时内会收到更新）',
      showCancel: false
    });
  }
});
