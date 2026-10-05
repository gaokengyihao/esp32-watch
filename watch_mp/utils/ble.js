/*
 * utils/ble.js —— 手表蓝牙封装（对应固件 main/ble_watch.c）
 *
 * 手表端服务/特征值：
 *   0xFF00 服务
 *   0xFF01 读        固件版本      "1.6"
 *   0xFF02 读 + 通知  状态数据      "fw=1.6;up=123s;step=0;bat=--"
 *   0xFF03 写        命令         "reboot"
 *   0xFF04 写        固件数据(OTA)  [4字节总长度] + 若干 [2字节序号+数据] 块
 *
 * 传输协议（自己实现的"窗口 + 补发"，为了又快又不丢）：
 *   - 用"无响应写"连发（不等 ATT 回应），速度是"等回应"的几十倍
 *   - 手表每 100ms 通知 "otaq=N"（它期待的下一个包号）：
 *       手机按 N 做流控（最多领先一个窗口）；发现 N 落后就补发丢的包
 *   - 每块数据前的 2 字节序号让手表能识别丢包/重复包
 *   - 屏障误判时：读 0xFF02 状态（请求-应答，100% 可靠）拿 "otaq=N" 精确补发，
 *     不再盲目重发一整批 —— 实测这是 iOS 升级速度的最大杀手
 *   手表通知一览：ota=N% 写出进度；otaq=N 流控回报；otag=N 请从 N 补发
 */

const DEVICE_NAME = 'ESP32-Watch';

const UUID = {
  svc: 'FF00',
  version: 'FF01',
  status: 'FF02',
  cmd: 'FF03',
  ota: 'FF04'
};

// 把各种写法的 UUID 都归一成 4 位短码（"0000FF01-...": -> "FF01"）
function shortUuid(u) {
  const h = (u || '').toUpperCase().replace(/[^0-9A-F]/g, '');
  return h.length >= 8 ? h.slice(4, 8) : h;
}

function ab2str(buf) {
  const u8 = new Uint8Array(buf);
  let s = '';
  for (let i = 0; i < u8.length; i++) s += String.fromCharCode(u8[i]);
  return s;
}

class WatchBle {
  constructor() {
    this.deviceId = null;
    this.serviceId = null;
    this.chars = {};
    this.connected = false;
    this.isIOS = wx.getSystemInfoSync().platform === 'ios';
    /* 每个数据块里"固件内容"的字节数（另加 2 字节序号）
     * iOS：MTU 185 -> 净荷 182，用 178+2=180 稳（官方"建议 20"太保守，慢 9 倍）*/
    this.chunkSize = this.isIOS ? 178 : 240;
    this.onLog = null;
    this.onStatus = null;      // 回调：收到 0xFF02 的内容（数据/进度）
    this.notifyOn = false;
    this.watchPct = -1;        // 手表回报的升级进度（界面显示用）
    this.otaState = { acked: 0, gap: -1 };   // 手表回报的接收进度（流控/补发用）
    this.otaSeen = false;      // 是否收到过通知（只当辅助：有的手机通知不可靠）
    this.otaResult = null;     // 手表校验结果：'ok' / 'fail'
    this._lastReportAt = 0;    // 最近一次收到手表回报的时间（判断通知还活不活）
    this._writeFails = 0;      // 无响应写失败计数（手机蓝牙队列满会给 fail）
    this._connHooked = false;  // 有没有挂过"连接断开"监听
    this._statusCb = null;
  }

  /** 页面收到手表进度通知时调用（界面显示用） */
  setWatchProgress(pct) {
    this.watchPct = pct;
  }

  /** 解析手表的一切回包（通知 or "读状态"的结果）
   *  - "otaq=N" 或状态串里的 ";otaq=N"：手表期待的下一个包号（流控/补发依据）
   *  - 状态串里的 ";otas=ok/fail"：校验结果
   *  - 通知里的 "otag=N"：丢包请从 N 补发（通知通道能用时的加速信号） */
  _handleReport(text) {
    this._lastReportAt = Date.now();
    let n = NaN;
    if (text.indexOf('otaq=') === 0) {
      n = parseInt(text.slice(5), 10);
    } else {
      const i = text.indexOf(';otaq=');
      if (i >= 0) n = parseInt(text.slice(i + 6), 10);
    }
    if (!isNaN(n) && n > this.otaState.acked) this.otaState.acked = n;

    if (text.indexOf('otas=fail') >= 0) this.otaResult = 'fail';
    else if (text.indexOf('otas=ok') >= 0) this.otaResult = 'ok';

    if (text.indexOf('otag=') === 0) {           // 丢包：请从第 n 包重发
      const g = parseInt(text.slice(5), 10);
      if (!isNaN(g)) this.otaState.gap = g;
    } else if (text.indexOf('ota=') === 0) {
      this.otaSeen = true;                       // 收到通知了（辅助信号）
      if (text === 'ota=0%') {                   // 新一轮开始：进度基线清零
        this.otaState.acked = 0;
        this.otaState.gap = -1;
      }
    }
  }

  _sleep(ms) {
    return new Promise((r) => setTimeout(r, ms));
  }

  log(s) {
    if (this.onLog) this.onLog(s);
  }

  // ---------------- 扫描 / 连接 ----------------

  openAdapter() {
    return new Promise((resolve, reject) => {
      wx.openBluetoothAdapter({ success: resolve, fail: reject });
    });
  }

  scan(timeoutMs = 15000) {
    return new Promise((resolve, reject) => {
      let done = false;
      const timer = setTimeout(() => {
        if (!done) { done = true; this.stopScan(); reject(new Error('没找到手表（先点一下手表屏幕叫醒它）')); }
      }, timeoutMs);

      wx.onBluetoothDeviceFound((res) => {
        (res.devices || []).forEach((d) => {
          const name = d.name || d.localName || '';
          if (!done && name.indexOf(DEVICE_NAME) >= 0) {
            done = true;
            clearTimeout(timer);
            this.stopScan();
            resolve(d);
          }
        });
      });

      wx.startBluetoothDevicesDiscovery({
        allowDuplicatesKey: false,
        fail: reject
      });
    });
  }

  stopScan() {
    try { wx.stopBluetoothDevicesDiscovery({}); } catch (e) {}
  }

  connect(deviceId) {
    return new Promise((resolve, reject) => {
      this.deviceId = deviceId;
      wx.createBLEConnection({
        deviceId,
        success: () => {
          if (this.isIOS) {
            this.discover().then(resolve).catch(reject);
          } else {
            // Android：试着把 MTU 拉大（iOS 不支持这个接口）
            wx.setBLEMTU({
              deviceId,
              mtu: 512,
              complete: () => this.discover().then(resolve).catch(reject)
            });
          }
        },
        fail: reject
      });
    });
  }

  discover() {
    return new Promise((resolve, reject) => {
      wx.getBLEDeviceServices({
        deviceId: this.deviceId,
        success: (sres) => {
          const svc = (sres.services || []).find((s) => shortUuid(s.uuid) === UUID.svc);
          if (!svc) { reject(new Error('没找到手表的 0xFF00 服务')); return; }
          this.serviceId = svc.uuid;
          wx.getBLEDeviceCharacteristics({
            deviceId: this.deviceId,
            serviceId: this.serviceId,
            success: (cres) => {
              (cres.characteristics || []).forEach((c) => {
                this.chars[shortUuid(c.uuid)] = c.uuid;
              });
              this.connected = true;

              // 连接断开监听（手表升级完会重启=断连，用来判断升级成功）
              if (!this._connHooked && wx.onBLEConnectionStateChange) {
                this._connHooked = true;
                wx.onBLEConnectionStateChange((res) => {
                  if (res.deviceId === this.deviceId && !res.connected) {
                    this.connected = false;
                  }
                });
              }

              wx.onBLECharacteristicValueChange((r) => {
                if (shortUuid(r.characteristicId) === UUID.status) {
                  const text = ab2str(r.value);
                  this._handleReport(text);
                  if (this.onStatus) this.onStatus(text);
                }
              });
              resolve();
            },
            fail: reject
          });
        },
        fail: reject
      });
    });
  }

  disconnect() {
    this.connected = false;
    this.notifyOn = false;
    if (this.deviceId) {
      try { wx.closeBLEConnection({ deviceId: this.deviceId }); } catch (e) {}
    }
    this.deviceId = null;
  }

  // ---------------- 读 / 订阅 ----------------

  /** 读特征值（带超时；读结果通过 value-change 事件回来） */
  read(chrShort, timeoutMs = 1500) {
    return new Promise((resolve, reject) => {
      const chr = this.chars[chrShort];
      if (!chr) { reject(new Error('没有特征值 ' + chrShort)); return; }

      let done = false;
      let timer = null;
      const finish = (err, val) => {
        if (done) return;
        done = true;
        if (timer) clearTimeout(timer);
        wx.offBLECharacteristicValueChange(handler);
        if (err) reject(err); else resolve(val);
      };
      const handler = (r) => {
        if (shortUuid(r.characteristicId) === chrShort) finish(null, ab2str(r.value));
      };
      wx.onBLECharacteristicValueChange(handler);
      timer = setTimeout(() => finish(new Error('读超时')), timeoutMs);

      wx.readBLECharacteristicValue({
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: chr,
        fail: (e) => finish(e || new Error('读失败'))
      });
    });
  }

  readVersion() { return this.read(UUID.version); }
  readStatus() { return this.read(UUID.status); }

  startNotify() {
    return new Promise((resolve, reject) => {
      wx.notifyBLECharacteristicValueChange({
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: this.chars[UUID.status],
        state: true,
        success: () => { this.notifyOn = true; resolve(); },
        fail: reject
      });
    });
  }

  // ---------------- 写（带排队，供 OTA 用） ----------------

  writeCmd(text) {
    return new Promise((resolve, reject) => {
      const buf = new Uint8Array(text.length);
      for (let i = 0; i < text.length; i++) buf[i] = text.charCodeAt(i);
      wx.writeBLECharacteristicValue({
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: this.chars[UUID.cmd],
        value: buf.buffer,
        success: resolve,
        fail: reject
      });
    });
  }

  /**
   * 把整段固件写进 0xFF04（先 4 字节总长度，再数据块）
   *
   * ★ 最终方案：分批连发 + "屏障"确认（不依赖通知，不依赖读，全靠握手）
   *   1) 写 4 字节总长度（发两遍，幂等）；然后用命令 "q"（0xFF03，带响应写）
   *      确认手表已开始 —— 命令是"请求-应答"，100% 可靠 ✓
   *   2) 数据块 = [2字节序号][固件内容]，一批连发 N 包（无响应写，快）
   *   3) 每批发完写命令 "b<下一包号>"（屏障）：手表检查是否全部到齐，
   *        到齐 -> ATT 成功；没到齐 -> ATT 错误（手机收到错误就补发整批）✓
   *   4) 连续成功就自动加大批次 N（16→32→64），失败就减半（自适应）✓
   *   5) 数据发完后命令 "r" 查询校验结果（成功=校验通过）；手表重启断连也算成功
   *
   * @param {ArrayBuffer} buffer 固件内容
   * @param {(sent:number,total:number,kbps:string|null,extra:string)=>void} onProgress
   *        sent = 手表"已确认"的字节数（真实进度）
   */
  async uploadFirmware(buffer, onProgress) {
    const total = buffer.byteLength;
    const CHUNK = this.chunkSize;                 // 每包固件内容字节数
    const otaChr = this.chars[UUID.ota];

    /* 先分包：保证每个数据块总长 ≥5 字节，避免和 4 字节的"总长度头"混淆
     * （当最后一包只剩 1~2 字节数据时，从前一包借几字节过来）*/
    const bounds = [];                            // [start, end)
    let off = 0;
    while (off < total) {
      let n = Math.min(CHUNK, total - off);
      const rem = total - (off + n);
      if (rem > 0 && rem <= 2) n -= (4 - rem);    // 让最后一包数据 ≥6 字节
      bounds.push([off, off + n]);
      off += n;
    }
    const totalChunks = bounds.length;

    this.otaState = { acked: 0, gap: -1 };
    this.otaSeen = false;
    this.otaResult = null;
    this.sentAllOnce = false;                     // 本机是否已完整发过至少一遍（"升级失败"误报的判据）

    // 1) 总长度头 + 命令 "q" 确认开始（头发两遍，重复发是幂等的）
    const header = new Uint8Array(4);
    new DataView(header.buffer).setUint32(0, total, true);
    let started = false;
    for (let attempt = 0; attempt < 3 && !started; attempt++) {
      this._writeOnce(header.buffer, otaChr).catch(() => {});
      await this._sleep(100);
      this._writeOnce(header.buffer, otaChr).catch(() => {});
      const t0 = Date.now();
      while (!started && Date.now() - t0 < 2000) {
        try { await this._cmdWithTimeout('q', 900); started = true; } catch (e) {}
        if (!started) await this._sleep(200);
      }
    }
    if (!started) throw new Error('手表没有开始升级（命令无响应）');

    /* 2) 多遍扫传：每一遍都从第 0 包扫到最后一包；一遍扫完就问手表"校验过了吗"（命令 r）
     *    —— 为什么这么设计：这台手机上"屏障"的回执偶尔会撒谎（假成功），
     *       所以"App 自以为发完了"永远不能当真；唯一可信的终点是
     *       "手表校验通过"（r 成功）。流程：扫一遍 -> 问 -> 没好就从 0 再扫一遍。
     *       每多扫一遍，上一遍丢掉的个别包就有一次补上的机会，最终必定收敛 ✓ */
    let shownBytes = 0;                           // 显示用进度（跨遍只前进不后退）
    let speedT0 = Date.now();                     // 实时速度：上次采样时间
    let speedBytes = 0;                           // 实时速度：上次采样的已确认字节数
    let sentAllOnce = false;                      // 有没有完整发出过至少一整遍
    let lastTick = Date.now();
    const start = Date.now();
    let pass = 0;

    while (pass < 6) {
      pass++;
      let N = 32;                                 // 每批包数（自适应 4~64）
      let nextSend = 0;                           // 下一批从哪发
      let confirmed = 0;                          // 本遍已确认收满的包数
      let confirmedSafe = 0;                      // 保守确认位置（屏障容差外的兜底补发起点）
      let lastAdvance = Date.now();               // 上次"确认前进"的时间
      let lastRewind = 0;                         // 上次"自愈回退"的时间
      this.log(`开始第 ${pass} 遍扫传`);

      while (confirmed < totalChunks) {
        if (this.otaResult === 'ok') return 'ok'; // 通知/读能带回成功信号的话直接用
        if (Date.now() - start > 20 * 60 * 1000) throw new Error('传输超时（超过 20 分钟）');

        /* ★ 自愈：7 秒没前进 -> 大退 512 包重新扫（本遍内的快速修补） */
        if (Date.now() - Math.max(lastAdvance, lastRewind) > 7000) {
          const back = Math.min(confirmed, 512);
          confirmed -= back;
          nextSend = confirmed;
          N = 32;
          lastRewind = Date.now();
          this.log(`第 ${pass} 遍卡住自愈：退回第 ${confirmed} 包重新扫`);
        }

        const burstStart = nextSend;
        const burstEnd = Math.min(nextSend + N, totalChunks);

        // 连发整批（无响应写，不等回应 —— 快的原因；可靠性靠后面的屏障）
        const send = (seq) => {
          const s = bounds[seq][0];
          const e = bounds[seq][1];
          const u8 = new Uint8Array(2 + (e - s));
          u8[0] = seq & 0xff;
          u8[1] = (seq >> 8) & 0xff;
          u8.set(new Uint8Array(buffer, s, e - s), 2);
          this._writeOnce(u8.buffer, otaChr).catch(() => {});
        };
        for (let seq = burstStart; seq < burstEnd; seq++) send(seq);

        /* ★ 小技巧："冲刷包"。iOS 对一批无响应写的"最后一包"会压着稍后才发，
         *   导致屏障检查时刚好缺最后 1 包；再补发下一批的第一包把它顶出去 ✓ */
        if (burstEnd < totalChunks) send(burstEnd);
        await this._sleep(120);

        // 屏障：告诉手表"这批应该都到了"；差超过 4 个包它会回错误
        let ok = false;
        try { await this._cmdWithTimeout('b' + burstEnd, 4000); ok = true; } catch (e) {}

        if (ok) {
          confirmed = burstEnd;
          nextSend = burstEnd;
          lastAdvance = Date.now();
          /* 屏障有 ±4 包容差：保守确认位置 = burstEnd-4，兜底补发时最多多补 4 包 */
          confirmedSafe = Math.max(confirmedSafe, burstEnd - 4);
          /* ★ 成功就尽快把批次恢复到更大；但上限 32 包
           *   —— 实测 64 包一冲，手机蓝牙队列扛不住，会被误判丢一大片 */
          if (N < 32) N = Math.min(N * 2, 32);
        } else {
          N = Math.max(Math.floor(N / 2), 8);                     // 不顺就缩小批次（最低 8）
          let ack = this.otaState.acked;

          /* ★ 精确问位置：读 0xFF02 状态（请求-应答，100% 可靠；状态串里带 ;otaq=N）
           *   以前"盲目回退一整批重发"是最大的速度杀手——重发的全是已经收到的包 */
          if (ack === 0 || ack >= burstEnd) {
            try { await this.readStatus(); } catch (e) {}
            ack = this.otaState.acked;
          }
          if (this.otaState.gap >= 0) {                 // 手表明确说"从第 N 包补发"
            ack = this.otaState.gap;
            this.otaState.gap = -1;
          }

          if (ack >= burstEnd) {
            /* 手表其实全收到了（屏障误判）：直接继续，不要重发 ✓ */
            confirmed = Math.min(ack, totalChunks);
            nextSend = confirmed;
            lastAdvance = Date.now();
            this.log(`屏障误判：手表已收到 ${ack} 包，继续`);
          } else if (ack > 0) {
            nextSend = ack;                             // 精确缺口：只补真缺的包
            this.log(`屏障失败，从第 ${nextSend} 包精确补发（批次改为 ${N}）`);
          } else {
            nextSend = Math.max(0, confirmedSafe - 4);  // 兜底：回到保守确认位置
            this.log(`屏障失败（没问到位置），从第 ${nextSend} 包补发（批次改为 ${N}）`);
          }
        }

        if (nextSend >= totalChunks) {
          sentAllOnce = true;                     // 这一遍把全部包都发出过了
          this.sentAllOnce = true;
        }

        if (!this.connected) {
          /* 断连：完整发过至少一遍之后断连 = 手表收齐、校验、重启的正常现象 ✓
           * （没发完就断 = 真的断了） */
          if (sentAllOnce) return 'ok';
          throw new Error('蓝牙连接断开了');
        }

        const now = Date.now();
        if (now - lastTick >= 350) {
          let nb = confirmed >= totalChunks ? total : bounds[confirmed][0];
          if (nb < shownBytes) nb = shownBytes;   // 跨遍只前进不后退
          shownBytes = nb;

          /* 实时速度：按"手表确认到的字节数"算，界面显示 KB/s */
          let kbps = null;
          const dsec = (now - speedT0) / 1000;
          if (dsec >= 1) {
            kbps = String(Math.max(0, Math.round((nb - speedBytes) / dsec / 1024)));
            speedBytes = nb;
            speedT0 = now;
          }
          lastTick = now;
          if (onProgress) {
            const shownChunks = Math.min(totalChunks, Math.floor(nb / CHUNK));
            onProgress(nb, total, kbps, `第${pass}遍 确认${shownChunks}/${totalChunks}包 批次${N}`);
          }
        }
      }

      this.log(`第 ${pass} 遍扫完，问手表校验结果...`);

      // 3) 一遍扫完：问 "r"（手表收齐并校验通过才会成功）；最多问约 4 秒
      for (let i = 0; i < 12; i++) {
        if (this.otaResult === 'ok') return 'ok';
        if (this.otaResult === 'fail') throw new Error('手表校验失败');
        if (!this.connected) return 'ok';        // 手表重启了 = 校验通过 ✓
        try { await this._cmdWithTimeout('r', 1500); return 'ok'; } catch (e) {}
        await this._sleep(200);
      }
      if (onProgress) onProgress(shownBytes, total, null, `第${pass}遍没确认，重扫中...`);
    }
    throw new Error('扫了 6 遍手表都没确认（连接可能不稳，可重试）');
  }

  /** 写命令（0xFF03，带响应）并等回应；超时算失败 */
  _cmdWithTimeout(text, timeoutMs) {
    return Promise.race([
      this.writeCmd(text),
      new Promise((_, rej) => setTimeout(() => rej(new Error('命令超时')), timeoutMs))
    ]);
  }

  _writeOnce(data, chr) {
    return new Promise((resolve, reject) => {
      wx.writeBLECharacteristicValue({
        deviceId: this.deviceId,
        serviceId: this.serviceId,
        characteristicId: chr,
        value: data,
        success: resolve,
        fail: reject
      });
    });
  }
}

module.exports = WatchBle;
