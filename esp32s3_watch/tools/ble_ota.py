"""
ble_ota.py —— 用电脑蓝牙给手表传新固件（蓝牙 OTA 测试工具）

用法（电脑要有蓝牙，没蓝牙就买个 USB 蓝牙 dongle，十几块）：
    pip install bleak
    python tools/ble_ota.py build/esp32s3_watch.bin

流程（和 ble_watch.c 里写的协议一致）：
    1) 扫描设备名 "ESP32-Watch"（手表要先点一下屏幕叫醒它）
    2) 读版本 0xFF01、读状态 0xFF02，并订阅 0xFF02 看进度
    3) 往 0xFF04 写 4 字节总长度，然后把固件按 244 字节一块块传过去
    4) 手表收满 -> 校验 -> 写进另一块分区 -> 自动重启进新固件
"""
import asyncio
import sys
from pathlib import Path

from bleak import BleakClient, BleakScanner

NAME = "ESP32-Watch"
UUID_SVC = "0000ff00-0000-1000-8000-00805f9b34fb"
CHR_VERSION = "0000ff01-0000-1000-8000-00805f9b34fb"
CHR_STATUS = "0000ff02-0000-1000-8000-00805f9b34fb"
CHR_CMD = "0000ff03-0000-1000-8000-00805f9b34fb"
CHR_OTA = "0000ff04-0000-1000-8000-00805f9b34fb"

CHUNK = 244          # 每块字节数（MTU 512 时安全值）


def on_status(_sender, data: bytearray):
    print(f"  [手表] {data.decode(errors='replace')}")


async def main(bin_path: str):
    data = Path(bin_path).read_bytes()
    print(f"固件文件: {bin_path}")
    print(f"固件大小: {len(data)} 字节\n")

    print("正在扫描手表（先点一下手表屏幕叫醒它）...")
    dev = await BleakScanner.find_device_by_name(NAME, timeout=20.0)
    if dev is None:
        print("没找到手表：确认①手表醒着 ②手表上电源灯亮 ③没被别的设备连着")
        return 1

    print(f"找到: {dev.name} [{dev.address}]")
    async with BleakClient(dev) as cli:
        print("已连接 ✓\n")

        ver = await cli.read_gatt_char(CHR_VERSION)
        st = await cli.read_gatt_char(CHR_STATUS)
        print(f"手表版本 : {ver.decode(errors='replace')}")
        print(f"手表状态 : {st.decode(errors='replace')}\n")

        print("开始传固件（可随时 Ctrl+C 中断，手表不会变砖）")
        await cli.start_notify(CHR_STATUS, on_status)

        await cli.write_gatt_char(CHR_OTA, len(data).to_bytes(4, "little"),
                                  response=True)

        sent = 0
        for i in range(0, len(data), CHUNK):
            block = data[i:i + CHUNK]
            await cli.write_gatt_char(CHR_OTA, block, response=False)
            sent += len(block)
            if sent % (CHUNK * 50) < CHUNK:      # 大约每 12KB 刷新一下进度
                print(f"\r  传输中 {sent * 100 // len(data)}%", end="", flush=True)

        print(f"\r  传输完成 100%")
        print("\n等手表校验+重启（约 5 秒）...")
        await asyncio.sleep(6)

    print("完成！手表重启后重新连上看版本号应该已经变了 ✓")
    return 0


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "build/esp32s3_watch.bin"
    sys.exit(asyncio.run(main(path)))
