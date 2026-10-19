"""串口 I/O 独立线程；UI 不阻塞。每次打开都从停止发送状态开始。"""
import queue
import threading
import time
import serial
from PySide6.QtCore import QThread, Signal
from protocol import FrameStream, parse_command


class SerialWorker(QThread):
    received = Signal(bytes, object)
    transmitted = Signal(bytes)
    failed = Signal(str)
    opened = Signal()
    diagnostics = Signal(int)

    def __init__(self, port, parent=None):
        super().__init__(parent)
        self.port = port
        self.halt = threading.Event()
        self.pending = queue.Queue(maxsize=1)

    def set_frame(self, frame):
        try: self.pending.get_nowait()
        except queue.Empty: pass
        try: self.pending.put_nowait(frame)
        except queue.Full: pass

    def run(self):
        try:
            with serial.Serial(None, 9600, bytesize=8, parity=serial.PARITY_NONE,
                               stopbits=serial.STOPBITS_TWO, timeout=.01, write_timeout=.2,
                               xonxoff=False, rtscts=False, dsrdtr=False) as port:
                port.dtr = port.rts = False
                port.port = self.port
                port.open()
                port.reset_input_buffer()
                parser = FrameStream()
                frame = None
                updated_at = 0
                next_tx = 0
                self.opened.emit()
                while not self.halt.is_set():
                    try:
                        frame = self.pending.get_nowait()
                        updated_at = time.monotonic()
                    except queue.Empty: pass
                    now = time.monotonic()
                    data = port.read(min(256, max(1, port.in_waiting)))
                    if data:
                        old = parser.errors
                        for raw in parser.feed(data, time.monotonic()):
                            try: cmd = parse_command(raw)
                            except ValueError:
                                parser.errors += 1
                                continue
                            self.received.emit(raw, cmd)
                        if parser.errors != old: self.diagnostics.emit(parser.errors)
                    if frame is not None and now - updated_at < .3 and now >= next_tx:
                        written = port.write(frame)
                        if written != len(frame): raise serial.SerialTimeoutException('报文未完整发送')
                        self.transmitted.emit(frame)
                        next_tx = now + .12
        except (serial.SerialException, OSError, ValueError) as exc:
            self.failed.emit(str(exc))

    def stop(self):
        self.halt.set()
