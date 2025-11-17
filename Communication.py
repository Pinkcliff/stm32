import socket
import time
from typing import List, Dict


# --------------------------
# 核心工具函数：Modbus RTU帧处理（不变）
# --------------------------
def modbus_crc(data: List[int]) -> List[int]:
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return [crc & 0xFF, (crc >> 8) & 0xFF]


def build_rtu_request(slave_addr: int, start_reg: int, reg_count: int, func_code: int = 0x04) -> bytes:
    # 新增func_code参数，支持0x03（保持寄存器）和0x04（输入寄存器）切换
    frame = [
        slave_addr,
        func_code,  # 功能码可配置
        (start_reg >> 8) & 0xFF,
        start_reg & 0xFF,
        (reg_count >> 8) & 0xFF,
        reg_count & 0xFF
    ]
    crc = modbus_crc(frame)
    frame.extend(crc)
    return bytearray(frame)


def parse_rtu_response(response_bytes: bytes) -> Dict:
    response = list(response_bytes)
    if len(response) < 4:
        return {"error": "响应帧过短"}

    slave_addr = response[0]
    func_code = response[1]
    data = response[2:-2]
    received_crc = response[-2:]

    calculated_crc = modbus_crc(response[:-2])
    if received_crc != calculated_crc:
        return {"error": f"CRC校验失败（接收: {received_crc}，计算: {calculated_crc}）"}

    # 支持0x03和0x04功能码解析
    if func_code in [0x03, 0x04]:
        if len(data) < 1:
            return {"error": f"功能码{func_code:02X}响应数据为空"}
        byte_count = data[0]
        registers = []
        for i in range(1, len(data), 2):
            if i + 1 > len(data):
                break
            reg_value = (data[i] << 8) | data[i + 1]
            registers.append(reg_value)
        return {
            "slave_addr": slave_addr,
            "func_code": func_code,
            "registers": registers,
            "valid": True
        }
    else:
        return {"error": f"不支持的功能码：0x{func_code:02X}"}


# --------------------------
# 优化后的主逻辑：循环接收+重试机制+持续读取
# --------------------------
def connect_and_communicate():
    # 1. 设备参数（务必与设备手册一致！）
    DEVICE_IP = "192.168.0.7"    # 设备IP
    DEVICE_PORT = 8234           # 设备Modbus端口（确认是否正确）
    SLAVE_ADDR = 1               # 设备从站地址（确认是否正确）
    FUNC_CODE = 0x04             # 功能码（0x03=保持寄存器，0x04=输入寄存器，确认！）
    START_REG = 0                # 起始寄存器地址（确认！）
    REG_COUNT = 2                # 读取寄存器数量（确认！）

    # 2. 通信参数优化
    TIMEOUT = 10                  # 超时时间从5秒延长到10秒（应对设备慢响应）
    BUFFER_SIZE = 1024
    RETRY_COUNT = 3               # 重试3次（网络波动时提高成功率）
    RETRY_DELAY = 1               # 每次重试间隔1秒
    READ_INTERVAL = 2             # 每次读取间隔2秒

    print(f"开始持续读取传感器数据...")
    print(f"设备地址: {DEVICE_IP}:{DEVICE_PORT}")
    print(f"读取间隔: {READ_INTERVAL}秒")
    print(f"按 Ctrl+C 停止读取\n")

    try:
        while True:
            # 3. 重试机制
            success = False
            for retry in range(RETRY_COUNT):
                try:
                    # 创建TCP socket（每次重试重新创建连接，避免旧连接干扰）
                    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
                        sock.settimeout(TIMEOUT)
                        print(f"[{time.strftime('%H:%M:%S')}] 第{retry+1}次尝试连接...")
                        sock.connect((DEVICE_IP, DEVICE_PORT))
                        print(f"[{time.strftime('%H:%M:%S')}] 连接成功")

                        # 构建请求
                        request = build_rtu_request(
                            slave_addr=SLAVE_ADDR,
                            start_reg=START_REG,
                            reg_count=REG_COUNT,
                            func_code=FUNC_CODE  # 传入配置的功能码
                        )

                        # --------------------------
                        # 优化：循环接收，直到拿到完整响应
                        # --------------------------
                        response_bytes = b""  # 存储完整响应
                        start_time = time.time()
                        while True:
                            # 读取缓冲区数据
                            chunk = sock.recv(BUFFER_SIZE)
                            if chunk:
                                response_bytes += chunk
                                # 判断是否为完整的Modbus RTU帧：
                                # 最小长度=从站(1)+功能码(1)+数据(至少1)+CRC(2) → 5字节
                                if len(response_bytes) >= 5:
                                    # 从响应中提取数据长度（功能码后1字节是数据字节数）
                                    data_len = response_bytes[2]  # 第3字节是数据字节数
                                    # 完整帧长度=从站(1)+功能码(1)+数据字节数(1)+数据(data_len)+CRC(2)
                                    full_frame_len = 1 + 1 + 1 + data_len + 2
                                    if len(response_bytes) >= full_frame_len:
                                        break  # 拿到完整帧，退出循环
                            # 超时判断
                            if time.time() - start_time > TIMEOUT:
                                raise socket.timeout(f"接收响应超时（{TIMEOUT}秒）")
                            time.sleep(0.01)  # 短延时，避免CPU占用过高

                        # 解析响应
                        parsed_data = parse_rtu_response(response_bytes)
                        if "error" in parsed_data:
                            print(f"[{time.strftime('%H:%M:%S')}] 解析失败：{parsed_data['error']}")
                            continue  # 解析失败，重试

                        # 转换物理量
                        registers = parsed_data["registers"]
                        if len(registers) >= REG_COUNT:
                            temp_raw = registers[0]
                            pressure_raw = registers[1]
                            temperature = ((temp_raw / 249) - 4) * 7.5 - 40
                            pressure = ((pressure_raw / 249) - 4) * 7.5

                            # 优化显示格式，带时间戳
                            print(f"[{time.strftime('%H:%M:%S')}] " + "="*40)
                            print(f"[{time.strftime('%H:%M:%S')}] 温度：{temperature:6.2f}℃  |  气压：{pressure:6.2f}kPa")
                            print(f"[{time.strftime('%H:%M:%S')}] " + "="*40)
                            success = True
                            break  # 成功，跳出重试循环

                except socket.timeout as e:
                    print(f"[{time.strftime('%H:%M:%S')}] 超时错误：{e}")
                except ConnectionRefusedError:
                    print(f"[{time.strftime('%H:%M:%S')}] 连接被拒绝：请检查IP:{DEVICE_IP}、端口:{DEVICE_PORT}是否正确")
                    break  # 端口/IP错，重试也没用，直接退出
                except Exception as e:
                    print(f"[{time.strftime('%H:%M:%S')}] 第{retry+1}次通信异常：{str(e)}")

                # 重试间隔
                if retry < RETRY_COUNT - 1:
                    print(f"[{time.strftime('%H:%M:%S')}] 等待{RETRY_DELAY}秒后重试...")
                    time.sleep(RETRY_DELAY)

            # 如果所有重试都失败
            if not success:
                print(f"[{time.strftime('%H:%M:%S')}] {RETRY_COUNT}次尝试均失败，请检查设备配置或网络")

            # 等待下一次读取
            print(f"[{time.strftime('%H:%M:%S')}] 等待{READ_INTERVAL}秒后进行下一次读取...")
            time.sleep(READ_INTERVAL)

    except KeyboardInterrupt:
        print(f"\n[{time.strftime('%H:%M:%S')}] 用户中断，程序退出")
    except Exception as e:
        print(f"\n[{time.strftime('%H:%M:%S')}] 程序异常退出：{str(e)}")


if __name__ == "__main__":
    connect_and_communicate()