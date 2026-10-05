/*
 * power_mgr.h —— 低功耗：空闲自动熄屏 + light sleep + 触摸/按键唤醒
 *
 * 面试考点：手表怎么省电？
 *   1) 屏幕是最耗电的（背光）-> 没人看就熄屏
 *   2) WiFi 不用就关（本项目对时完就关，见 wifi_time.c）
 *   3) CPU 空闲时进 light sleep：电流从几十 mA 掉到 1mA 级，
 *      靠"触摸中断 / 按键 / 定时器"唤醒，醒来立刻能用
 *   4) 状态灯也关掉（WS2812 待机也要 1mA 左右）
 *
 * 本模块做的事：
 *   - 15 秒没有触摸/按键 -> 熄屏 + 关灯 + esp_light_sleep_start()
 *   - 唤醒源：触摸 INT（GPIO7）、BOOT 键（GPIO0）、60 秒定时（保证能醒、也能看日志）
 *   - 刚醒来的 0.6 秒内，"触摸/按键"只用来点亮屏幕，不执行操作
 *     （不然睡醒第一下就会误进菜单/误触发）
 *   - OTA 这种"不能被打断"的活儿，用 power_mgr_keep_awake(true) 顶着不睡
 */
#pragma once

#include <stdbool.h>
#include "esp_err.h"

/* 初始化：配置唤醒源 + 起一个空闲检测任务（在 button_init 之后调） */
esp_err_t power_mgr_init(void);

/* 有"活动"（触摸/按键）时调用，重置空闲计时 */
void power_mgr_notify_activity(void);

/* 做重要的事（OTA 等）时禁止睡觉：true = 别睡 */
void power_mgr_keep_awake(bool on);

/* 手机 App 连上时调用：请求点亮屏幕（可能正处在睡眠的"广播窗口"里） */
void power_mgr_request_wake_screen(void);

/* 刚睡醒的保护期：这段时间内的触摸/按键只当"点亮屏幕" */
bool power_mgr_is_waking(void);
