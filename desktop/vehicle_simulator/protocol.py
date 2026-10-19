"""神龙锂电2号（20180915）。电脑作为控制器，仪表作为命令发送端。

速度反馈是轮周时间而非 km/h；多字节为大端；全帧 XOR 为零。
PDF 的 21 字节表误写长度20，驱动方式表写1~3而正文为0~2，以正文为准。
"""
from dataclasses import dataclass
from functools import reduce
import math


def checksum(data: bytes) -> int:
    return reduce(int.__xor__, data, 0)


def checked(data: bytes) -> bytes:
    return data + bytes([checksum(data)])


@dataclass(frozen=True)
class MeterCommand:
    drive: int
    assist: int
    control: int
    magnets: int
    diameter_tenth_inch: int
    sensitivity: int
    start_strength: int
    inner_magnets: int
    speed_limit: int
    current_limit: int
    undervoltage_tenth_v: int
    throttle_pwm: int
    control2: int
    assist_pwm: int | None = None

    @property
    def power(self): return bool(self.control & 0x80)

    @property
    def light(self): return bool(self.control & 0x20)

    @property
    def reverse(self): return bool(self.control2 & 0x80)

    @property
    def cruise_hold(self): return bool(self.control & 2)

    @property
    def cruise_toggle(self): return bool(self.control & 1)

    @property
    def circumference_mm(self): return self.diameter_tenth_inch * 2.54 * math.pi


def parse_command(frame: bytes) -> MeterCommand:
    if not 20 <= len(frame) <= 25 or frame[:1] != b'\x01' or frame[1] != len(frame) or frame[2] != 1 or checksum(frame):
        raise ValueError('仪表帧地址、长度、命令或 XOR 校验错误')
    u16 = lambda i: int.from_bytes(frame[i:i+2], 'big')
    c = MeterCommand(frame[3], frame[4], frame[5], frame[6], u16(7), frame[9], frame[10],
                     frame[11], frame[12], frame[13], u16(14), u16(16), frame[18],
                     frame[19] if len(frame) >= 21 else None)
    if c.drive > 2 or c.assist > 15 or c.throttle_pwm > 1000:
        raise ValueError('仪表驱动方式、助力或 PWM 超出协议范围')
    return c


def encode_command(c: MeterCommand) -> bytes:
    n = 21 if c.assist_pwm is not None else 20
    u16 = lambda v: int(v).to_bytes(2, 'big')
    data = bytes([1, n, 1, c.drive, c.assist, c.control, c.magnets]) + u16(c.diameter_tenth_inch)
    data += bytes([c.sensitivity, c.start_strength, c.inner_magnets, c.speed_limit, c.current_limit])
    data += u16(c.undervoltage_tenth_v) + u16(c.throttle_pwm) + bytes([c.control2])
    if c.assist_pwm is not None: data += bytes([c.assist_pwm])
    result = checked(data)
    parse_command(result)
    return result


def wheel_period(speed_kph: float, circumference_mm: float) -> int:
    if not math.isfinite(speed_kph) or not math.isfinite(circumference_mm) or circumference_mm <= 0:
        raise ValueError('车速和轮周必须有效')
    # Protocol recommends 3500~4000ms as stop timeout. >=4000 is treated as stopped.
    return min(3999, max(1, round(circumference_mm * 3.6 / speed_kph))) if speed_kph > 0 else 4000


def encode_feedback(*, speed_kph=0.0, circumference_mm=1276.74, current_a=0.0,
                    load=0, soc=80, remaining_km=45, status1=0, status2=0x80) -> bytes:
    if not math.isfinite(current_a) or not 0 <= current_a <= 1638.3:
        raise ValueError('电流范围为 0~1638.3A；负数编码未定义')
    if not 0 <= load <= 100 or not 0 <= soc <= 100 or not 0 <= remaining_km <= 65535:
        raise ValueError('负载、电量或剩余里程超出范围')
    current = round(current_a * 10) | 0x4000
    data = bytes([2, 14, 1, status1, status2]) + current.to_bytes(2, 'big') + bytes([load])
    data += wheel_period(speed_kph, circumference_mm).to_bytes(2, 'big') + bytes([soc])
    return checked(data + remaining_km.to_bytes(2, 'big'))


class FrameStream:
    """Bounded scanner for USB chunks, split frames, concatenated frames and noise.

    An incomplete candidate expires after a conservative 100ms host read gap;
    the MCU uses DMA/length validation. Invalid bytes never refresh link health.
    """
    def __init__(self, address=1, minimum=20, maximum=25):
        self.address, self.minimum, self.maximum = address, minimum, maximum
        self.buffer = bytearray()
        self.errors = 0
        self.last_at = None

    def feed(self, data: bytes, now: float) -> list[bytes]:
        if self.last_at is not None and now - self.last_at > .1 and self.buffer:
            self.errors += 1
            self.buffer.clear()
        self.last_at = now
        frames = []
        for byte in data:
            self.buffer.append(byte)
            while self.buffer:
                if self.buffer[0] != self.address:
                    del self.buffer[0]
                    self.errors += 1
                    continue
                if len(self.buffer) < 3: break
                n = self.buffer[1]
                if not self.minimum <= n <= self.maximum or self.buffer[2] != 1:
                    del self.buffer[0]
                    self.errors += 1
                    continue
                if len(self.buffer) < n: break
                f = bytes(self.buffer[:n])
                if checksum(f):
                    del self.buffer[0]
                    self.errors += 1
                    continue
                del self.buffer[:n]
                frames.append(f)
        return frames
