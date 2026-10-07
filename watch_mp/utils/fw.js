/*
 * utils/fw.js —— 固件来源（最新版本号 + 固件文件）
 *
 * 两种模式，改下面这行就能切：
 *   MODE = 'http'   开发阶段：电脑上的 HTTP 服务器（publish.bat 自动同步到 D:\esp32\ota_server）
 *   MODE = 'cloud'  正式模式：微信云开发（固件放云存储，不需要域名/备案，手机 4G 也能用）
 *
 * 云开发模式需要两步准备：
 *   1) 小程序后台 -> 开发 -> 云开发 -> 开通（选免费额度）
 *   2) 云开发控制台 -> 存储 -> 建个文件夹 firmware/，上传两个文件：
 *        firmware/version.txt            （内容就是版本号，比如 1.7）
 *        firmware/esp32s3_watch.bin      （固件本体）
 *      然后把它们的【文件ID】(cloud:// 开头) 填到下面两个常量里
 *      （权限：上传后在文件上右键 -> 权限设置 -> 所有用户可读）
 */

const MODE = 'http';        // 'http' 或 'cloud'

/* ---------- 模式一：电脑 HTTP 服务器 ---------- */
const SERVER = 'http://192.168.1.174:8000';   // 电脑局域网地址（ipconfig 查）；IP 变了要改

/* ---------- 模式二：微信云开发 ---------- */
const CLOUD_ENV_ID = '你的云环境ID';           // 云开发控制台 -> 设置 -> 环境ID
const CLOUD_FILE_VERSION = 'cloud://你的云环境ID.xxxx/firmware/version.txt';
const CLOUD_FILE_FIRMWARE = 'cloud://你的云环境ID.xxxx/firmware/esp32s3_watch.bin';

/* -------------------------------------------------- */

function getLatestVersion() {
  if (MODE === 'cloud') {
    return new Promise((resolve, reject) => {
      wx.cloud.downloadFile({
        fileID: CLOUD_FILE_VERSION,
        success: (r) => {
          wx.getFileSystemManager().readFile({
            filePath: r.tempFilePath,
            success: (f) => {
              // ArrayBuffer -> 字符串
              let s = '';
              const u8 = new Uint8Array(f.data);
              for (let i = 0; i < u8.length; i++) s += String.fromCharCode(u8[i]);
              resolve(s.trim());
            },
            fail: reject
          });
        },
        fail: reject
      });
    });
  }
  // http 模式
  return new Promise((resolve, reject) => {
    wx.request({
      url: SERVER + '/version.txt',
      timeout: 8000,
      success: (r) => resolve(String(r.data || '').trim()),
      fail: reject
    });
  });
}

/** 下载固件，返回 ArrayBuffer；onProgress(0~100) */
function downloadFirmware(onProgress) {
  if (MODE === 'cloud') {
    return new Promise((resolve, reject) => {
      const task = wx.cloud.downloadFile({
        fileID: CLOUD_FILE_FIRMWARE,
        success: (r) => {
          wx.getFileSystemManager().readFile({
            filePath: r.tempFilePath,
            success: (f) => resolve(f.data),
            fail: reject
          });
        },
        fail: reject
      });
      if (onProgress && task.onProgressUpdate) {
        task.onProgressUpdate((p) => onProgress(p.progress));
      }
    });
  }
  // http 模式
  return new Promise((resolve, reject) => {
    const task = wx.downloadFile({
      url: SERVER + '/esp32s3_watch.bin',
      timeout: 60000,
      success: (r) => {
        if (r.statusCode !== 200) {
          reject(new Error('HTTP ' + r.statusCode));
          return;
        }
        wx.getFileSystemManager().readFile({
          filePath: r.tempFilePath,
          success: (f) => resolve(f.data),
          fail: reject
        });
      },
      fail: reject
    });
    if (onProgress && task.onProgressUpdate) {
      task.onProgressUpdate((p) => onProgress(p.progress));
    }
  });
}

module.exports = { getLatestVersion, downloadFirmware, MODE, SERVER, CLOUD_ENV_ID };
