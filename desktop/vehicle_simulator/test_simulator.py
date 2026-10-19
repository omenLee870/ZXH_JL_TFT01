import sys
import os
os.environ.setdefault('QT_QPA_PLATFORM','offscreen')
import random
import subprocess
import time
import unittest
from unittest.mock import patch
from protocol import MeterCommand, encode_command, encode_feedback, parse_command, checksum, checked, FrameStream
from simulation import Simulation


def cmd(**kw):
    fields=dict(drive=2,assist=10,control=0xa0,magnets=46,diameter_tenth_inch=160,
                sensitivity=12,start_strength=1,inner_magnets=0,speed_limit=60,
                current_limit=20,undervoltage_tenth_v=420,throttle_pwm=0,control2=8)
    fields.update(kw)
    return MeterCommand(**fields)


class ProtocolTests(unittest.TestCase):
    def test_feedback_known_vector(self):
        self.assertEqual(encode_feedback(speed_kph=30,current_a=12.3,load=61,status2=0xa0),bytes.fromhex('02 0e 01 00 a0 40 7b 3d 00 99 50 00 2d 4f'))

    def test_stream_noise_split_and_extensions(self):
        for pwm in (None,100):
            f=encode_command(cmd(assist_pwm=pwm))
            self.assertEqual(f[1],20 if pwm is None else 21)
            for split in range(len(f)+1):
                parser=FrameStream()
                output=parser.feed(b'\xff\x00'+f[:split],1)+parser.feed(f[split:]+f,1.02)
                self.assertEqual(output,[f,f]);self.assertEqual(len(parser.buffer),0)
            for length in range(len(f),26):
                extended=checked(bytes([1,length,1])+f[3:-1]+bytes(length-len(f)))
                self.assertEqual(parse_command(extended).diameter_tenth_inch,160)

    def test_corruption_and_timeout(self):
        f=encode_command(cmd())
        for i in range(len(f)):
            for bit in range(8):
                broken=bytearray(f);broken[i]^=1<<bit
                with self.assertRaises(ValueError):parse_command(broken)
        parser=FrameStream();parser.feed(f[:5],1)
        self.assertEqual(parser.feed(f,1.2),[f]);self.assertGreater(parser.errors,0)

    def test_bounded_input(self):
        p=FrameStream();p.feed(bytes([1,25,1])*100000,1)
        self.assertLess(len(p.buffer),25)

    def test_simulation_gates(self):
        s=Simulation(throttle=100)
        s.step(.1,1,False);self.assertEqual(s.speed,0)
        s.key=True
        for _ in range(100):s.step(.1,1,False)
        self.assertAlmostEqual(s.speed,60)
        s.brake=True
        for _ in range(40):s.step(.1,1,False)
        self.assertEqual(s.speed,0)
        s.brake=False;s.command_received(cmd(control=0),1)
        s.step(.1,1,True);self.assertEqual(s.speed,0)
        s.command_received(cmd(control=0xa0),1);s.step(.1,1,True)
        self.assertTrue(s.light);self.assertGreater(s.speed,0)
        for _ in range(100):s.step(.1,3,True)
        self.assertEqual(s.speed,0);self.assertFalse(s.light)
        self.assertTrue(s.feedback()[4]&0x10)
        s.command_received(cmd(control2=0x88),4)
        for _ in range(20):s.step(.1,4,True)
        self.assertLessEqual(s.speed,6)
        s.charging=True
        for _ in range(20):s.step(.1,4,True)
        self.assertEqual(s.speed,0);self.assertTrue(s.feedback()[4]&8)

    def test_native_decoder_agreement(self):
        exe=os.environ.get('SHENLONG_TEST_EXE')
        if not exe:self.skipTest('Set SHENLONG_TEST_EXE to native protocol test executable')
        rng=random.Random(20261019);vectors=[];expected=[]
        for _ in range(200):
            speed=rng.uniform(2,100);amps=rng.uniform(0,40)
            f=encode_feedback(speed_kph=speed,current_a=amps,soc=80,status2=0x80)
            vectors.append(f.hex(' '))
            period=int.from_bytes(f[8:10],'big')
            expected.append((round(160*2.54*3.141592653589793*36/period),round(amps*10)*100))
        r=subprocess.run([exe,'--decode'],input='\n'.join(vectors)+'\n',text=True,capture_output=True,check=True)
        for row,(speed,current) in zip(r.stdout.splitlines(),expected):
            values=list(map(int,row.split()));self.assertEqual(values[:2],[speed,current])
        self.assertEqual(len(r.stdout.splitlines()),len(expected))


class DesktopTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from PySide6.QtWidgets import QApplication
        cls.app=QApplication.instance() or QApplication([])

    def test_controls_and_resize(self):
        from app import Window, STYLE
        self.app.setStyleSheet(STYLE)
        w=Window();w.show();self.app.processEvents()
        w.key_button.click();self.assertTrue(w.model.key)
        w.throttle.setValue(50)
        for _ in range(50):w.model.step(.1,1,False)
        self.assertGreater(w.model.speed,20)
        w.light_button.click();w.position_button.click();w.brake_button.click()
        w.turn.setCurrentText('双闪');w.tick();self.app.processEvents()
        self.assertTrue(w.model.light and w.model.brake and w.model.position)
        for size in [(1060,740),(1380,900),(1600,1000)]:
            w.resize(*size);self.app.processEvents();self.assertFalse(w.grab().isNull())
        w.received(encode_command(cmd()),cmd());self.assertEqual(w.rx_count,1)
        w.reset_model();self.assertFalse(w.model.key);self.assertEqual(w.model.speed,0)
        w.close()

    def test_serial_worker_wire_and_pause(self):
        from transport import SerialWorker
        class FakeSerial:
            instance=None
            def __init__(self,*a,**kw):
                self.kw=kw;self.in_waiting=0;self.writes=[];self.reads=[encode_command(cmd())];FakeSerial.instance=self
            def __enter__(self):return self
            def __exit__(self,*a):pass
            def open(self):pass
            def reset_input_buffer(self):pass
            def read(self,n):
                time.sleep(.005)
                return self.reads.pop() if self.reads else b''
            def write(self,f):self.writes.append((time.monotonic(),f));return len(f)
        with patch('transport.serial.Serial',FakeSerial):
            worker=SerialWorker('TEST');received=[];worker.received.connect(lambda raw,c:received.append(c))
            worker.start();time.sleep(.02)
            worker.set_frame(encode_feedback())
            end=time.monotonic()+.3
            while time.monotonic()<end:
                worker.set_frame(encode_feedback());self.app.processEvents();time.sleep(.01)
            worker.set_frame(None);time.sleep(.05);n=len(FakeSerial.instance.writes)
            time.sleep(.15);self.assertEqual(len(FakeSerial.instance.writes),n)
            worker.stop();self.assertTrue(worker.wait(1000));self.app.processEvents()
            self.assertTrue(received);self.assertGreaterEqual(n,2)
            self.assertEqual(FakeSerial.instance.kw['stopbits'],2)
            times=[x[0] for x in FakeSerial.instance.writes]
            self.assertTrue(all(b-a>=.11 for a,b in zip(times,times[1:])))


if __name__=='__main__':unittest.main(verbosity=2)
