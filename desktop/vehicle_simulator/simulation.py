"""可复现的整车模拟；协议状态和仅动画状态分开处理。"""
from dataclasses import dataclass, field
from protocol import MeterCommand, encode_feedback


@dataclass
class Simulation:
    key: bool = False
    brake: bool = False
    charging: bool = False
    light: bool = False
    position: bool = False
    turn: str = '关闭'
    reverse: bool = False
    follow_meter: bool = True
    speed: float = 0.0
    target_speed: float = 0.0
    throttle: int = 0
    current: float = 0.0
    soc: int = 80
    remaining: int = 45
    diameter: float = 16.0
    automatic: bool = True
    command: MeterCommand | None = None
    last_command: float = -1e10
    cruise: bool = False
    cruise_target: float = 0.0
    previous_toggle: bool = False
    feedback_communication_fault: bool = False
    faults: set = field(default_factory=set)

    def command_received(self, cmd: MeterCommand, now: float):
        self.command, self.last_command = cmd, now
        if self.follow_meter:
            if cmd.cruise_toggle and not self.previous_toggle:
                self.cruise = not self.cruise
                self.cruise_target = self.speed if self.speed >= 1 else 6
            self.previous_toggle = cmd.cruise_toggle
            if cmd.control & 8:
                self.cruise = cmd.cruise_hold
                self.cruise_target = self.speed if self.speed >= 1 else 6

    def meter_online(self, now): return self.command is not None and now - self.last_command < 1.0

    def step(self, dt: float, now: float, connected: bool):
        dt = max(0, min(dt, .1))
        active = self.key and not self.charging
        self.feedback_communication_fault = connected and not self.meter_online(now)
        throttle = self.throttle
        limit = 60
        if connected and self.follow_meter:
            online = self.meter_online(now)
            cmd = self.command
            active = active and online and cmd.power
            self.light = bool(active and cmd.light) if cmd else False
            self.reverse = bool(active and cmd.reverse) if cmd else False
            if cmd and online:
                # No physical throttle attached: UI knob is the virtual vehicle throttle.
                # A nonzero meter PWM overrides it; key/brake/timeout still gate motion.
                throttle = round(cmd.throttle_pwm / 10) if cmd.throttle_pwm else self.throttle
                limit = cmd.speed_limit or 60
        if not active or self.brake or self.faults:
            target = 0
            self.cruise = False
        else:
            target = min(limit, throttle * .6) if self.automatic else self.target_speed
            if self.cruise: target = self.cruise_target
            if self.reverse: target = min(target, 6)
        change = (18 if self.brake else 8) * dt
        self.speed += max(-change, min(change, target - self.speed))
        if self.speed < .03: self.speed = 0
        self.current = round((1.2 + throttle * .14) if active and target > 0 and not self.brake else 0, 1)

    def feedback(self):
        s1 = sum(bit for name, bit in [('hall',64),('throttle',32),('controller',16),('low',8),('phase',1)] if name in self.faults)
        if self.cruise: s1 |= 128 if self.cruise_target <= 6 else 4
        s2 = 0x80 | (0x20 if self.brake else 0) | (8 if self.charging else 0)
        if self.feedback_communication_fault: s2 |= 0x10
        # A meter which has never sent data is not a simulated communications fault in preview.
        circumference = self.diameter * 25.4 * 3.141592653589793
        return encode_feedback(speed_kph=self.speed, circumference_mm=circumference,
                               current_a=self.current, load=min(100, round(self.current / 20 * 100)),
                               soc=self.soc, remaining_km=self.remaining, status1=s1, status2=s2)
