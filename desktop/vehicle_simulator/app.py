"""ZXH Vehicle Lab — native Windows desktop simulator."""
import csv
from collections import deque
from datetime import datetime
import math
from pathlib import Path
import sys
import time

from PySide6.QtCore import Qt, QTimer, QPointF, QRectF, QSettings
from PySide6.QtGui import QColor, QFont, QPainter, QPainterPath, QPen, QLinearGradient, QPolygonF
from PySide6.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QPushButton, QComboBox, QCheckBox, QSlider, QDoubleSpinBox, QSpinBox,
    QFrame, QScrollArea, QSplitter, QTabWidget, QTableWidget, QTableWidgetItem,
    QHeaderView, QFileDialog, QMessageBox, QAbstractItemView)
from serial.tools import list_ports
from simulation import Simulation
from transport import SerialWorker

GREEN = '#a9f078'
STYLE = '''
QWidget { background: #111819; color: #e4edeb; font-family: 'Microsoft YaHei UI'; font-size: 12px; }
QMainWindow, QWidget#root { background: #0b1112; }
QFrame#panel { background: #141e20; border: 1px solid #283638; border-radius: 12px; }
QLabel { background: transparent; border: none; }
QLabel#muted { color: #82999a; font-size: 11px; }
QLabel#title { font-size: 24px; font-weight: 700; }
QLabel#section { font-size: 14px; font-weight: 600; }
QLabel#number { font-size: 28px; font-family: 'Consolas'; font-weight: 600; color: #a9f078; }
QPushButton { background: #233234; border: 1px solid #354749; border-radius: 7px; padding: 8px 12px; }
QPushButton:hover { background: #32484a; border-color: #73918a; }
QPushButton:checked, QPushButton#primary { background: #a9f078; color: #112018; border-color: #a9f078; font-weight: 600; }
QPushButton#danger { color: #ffb49b; border-color: #895443; }
QPushButton:disabled { color: #526263; background: #182123; border-color: #283638; }
QComboBox, QSpinBox, QDoubleSpinBox { background: #0c1416; border: 1px solid #344648; border-radius: 6px; padding: 6px; min-height: 20px; }
QComboBox QAbstractItemView { background: #152426; selection-background-color: #37533c; }
QCheckBox { spacing: 8px; padding: 5px 0; background: transparent; }
QCheckBox::indicator { width: 16px; height: 16px; border: 1px solid #526b6b; border-radius: 4px; background: #0d1617; }
QCheckBox::indicator:checked { background: #a9f078; border-color: #a9f078; }
QSlider::groove:horizontal { height: 5px; background: #2c3c3e; border-radius: 2px; }
QSlider::sub-page:horizontal { background: #a9f078; border-radius: 2px; }
QSlider::handle:horizontal { background: #c8ffa2; width: 14px; margin: -5px 0; border-radius: 7px; }
QTabWidget::pane { border: none; }
QTabBar::tab { background: #141e20; color: #839a9b; padding: 10px 16px; border-bottom: 2px solid #283638; }
QTabBar::tab:selected { color: #a9f078; border-bottom-color: #a9f078; }
QTableWidget { background: #0c1416; alternate-background-color: #101c1e; border: none; gridline-color: #223133; font-family: 'Consolas'; font-size: 11px; }
QHeaderView::section { background: #1a292c; color: #8ea5a6; border: none; padding: 6px; }
QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: #111b1d; width: 8px; }
QScrollBar::handle:vertical { background: #3b5253; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
'''


def label(text, name=None):
    w = QLabel(text)
    if name: w.setObjectName(name)
    return w


def panel():
    w = QFrame()
    w.setObjectName('panel')
    layout = QVBoxLayout(w)
    layout.setContentsMargins(16, 14, 16, 14)
    layout.setSpacing(10)
    return w, layout


class VehicleScene(QWidget):
    """Resolution independent scooter illustration and interactive component hotspots."""
    def __init__(self, model, on_component):
        super().__init__()
        self.model, self.on_component = model, on_component
        self.phase = 0.0
        self.clock = 0.0
        self.selected = '整车'
        self.setMinimumHeight(280)
        self.setMouseTracking(True)
        self.setCursor(Qt.CursorShape.PointingHandCursor)

    def mousePressEvent(self, event):
        s = min(self.width()/900, self.height()/410)
        x = (event.position().x() - (self.width()-900*s)/2)/s
        y = (event.position().y() - (self.height()-410*s)/2)/s
        if x > 650: name = '大灯'
        elif x < 260: name = '刹车'
        elif y < 200: name = '电池'
        else: name = '电机'
        self.selected = name
        self.on_component(name)

    def paintEvent(self, event):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)
        s = min(self.width()/900, self.height()/410)
        p.translate((self.width()-900*s)/2, (self.height()-410*s)/2)
        p.scale(s,s)
        m = self.model
        def line(x1,y1,x2,y2,color,width=1):
            p.setPen(QPen(QColor(color),width,Qt.PenStyle.SolidLine,Qt.PenCapStyle.RoundCap))
            p.drawLine(QPointF(x1,y1),QPointF(x2,y2))
        def box(x,y,w,h,color,r=0,stroke=None):
            p.setPen(QPen(QColor(stroke),1) if stroke else Qt.PenStyle.NoPen)
            p.setBrush(QColor(color)); p.drawRoundedRect(QRectF(x,y,w,h),r,r)
        def text(x,y,t,size=12,color='#8aa3a4',bold=False):
            p.setPen(QColor(color)); p.setFont(QFont('Microsoft YaHei UI',size,700 if bold else 400)); p.drawText(QPointF(x,y),t)
        # Technical grid and moving road.
        for x in range(40,900,40): line(x,70,x,350,'#1c2b2d',.7)
        for y in range(70,351,40): line(40,y,860,y,'#1c2b2d',.7)
        p.setPen(Qt.PenStyle.NoPen)
        p.setBrush(QColor('#0a1011')); p.drawEllipse(QRectF(180,315,550,28))
        line(40,343,860,343,'#3b5351',1)
        offset = self.phase % 90
        for x in range(-90,900,90): line(x-offset,363,x+35-offset,363,'#324344',2)
        # Wheels and rotating spokes.
        for x in (248,655):
            p.setPen(QPen(QColor('#0a0f10'),20)); p.setBrush(QColor('#1d2c2f')); p.drawEllipse(QPointF(x,278),53,53)
            p.setPen(QPen(QColor('#58716f'),2)); p.setBrush(Qt.BrushStyle.NoBrush); p.drawEllipse(QPointF(x,278),41,41)
            for i in range(8):
                a = math.radians(self.phase*2+i*45)
                line(x+math.cos(a)*13,278+math.sin(a)*13,x+math.cos(a)*39,278+math.sin(a)*39,'#7b9391',2)
            p.setBrush(QColor('#455f5c')); p.setPen(QPen(QColor('#a3c2b5'),2)); p.drawEllipse(QPointF(x,278),14,14)
        # Motor and chassis.
        line(248,278,390,279,'#3d5556',14)
        line(390,279,557,279,'#738884',12)
        line(619,188,655,278,'#718885',12)
        line(636,191,672,268,'#c4d5cc',5)
        box(297,246,270,24,'#203a35',9,'#476456')
        box(324,245,214,6,'#789b85',3)
        # Scooter body.
        path = QPainterPath(QPointF(198,234))
        path.cubicTo(206,183,238,166,295,166)
        path.lineTo(404,167); path.cubicTo(438,173,440,205,449,233)
        path.lineTo(540,233); path.cubicTo(567,231,568,193,577,160)
        path.lineTo(598,110); path.lineTo(645,126)
        path.cubicTo(650,167,625,184,622,205)
        path.lineTo(602,264); path.lineTo(355,264)
        path.cubicTo(338,244,320,234,300,236)
        path.cubicTo(269,217,231,215,198,234)
        grad = QLinearGradient(220,150,560,280)
        grad.setColorAt(0,QColor('#b3df9b')); grad.setColorAt(1,QColor('#487d67'))
        p.setBrush(grad); p.setPen(QPen(QColor('#c8efb2'),1.4)); p.drawPath(path)
        line(349,187,364,243,'#629271',2)
        line(567,236,594,160,'#2c594d',5)
        box(229,150,190,24,'#0b1214',11,'#6a8280')
        box(240,151,157,6,'#3f5251',3)
        # Battery cutaway.
        box(274,184,73,45,'#13292a',8,'#a3cc9c')
        for i in range(4): box(282+i*15,193,10,26,GREEN if m.key else '#526b61',3)
        text(368,211,'ZXH',15,'#1b4033',True)
        # Head and handlebar.
        line(601,116,615,86,'#6c8982',9)
        line(580,82,646,98,'#a5bfb1',7)
        box(576,74,32,11,'#0b1314',5)
        line(594,81,578,47,'#587670',3)
        box(558,35,42,14,'#243a38',7,'#728b80')
        box(609,106,52,34,'#244a40',12,'#b9d7a7')
        box(646,111,15,23,'#f7f8cb' if m.key and m.light else '#71847b',6)
        # Headlight beam.
        if m.key and m.light:
            beam = QLinearGradient(659,122,863,122)
            beam.setColorAt(0,QColor(231,255,185,85)); beam.setColorAt(1,QColor(231,255,185,0))
            p.setBrush(beam); p.setPen(Qt.PenStyle.NoPen)
            p.drawPolygon(QPolygonF([QPointF(661,115),QPointF(862,64),QPointF(862,220),QPointF(661,134)]))
        # Position and brake lamps.
        box(199,180,16,13,'#ff584f' if m.key and (m.position or m.brake) else '#613e3d',4)
        if m.key and m.brake:
            for k in range(3): line(188-k*9,183-k*3,184-k*9,193+k*3,'#ff6c5c',2)
        if m.key and m.position: line(587,189,579,215,'#ecf5c6',3)
        blink = int(self.clock*2) % 2 == 0
        if m.key and blink and m.turn != '关闭':
            if m.turn in ('左转','双闪'): box(221,176,13,8,'#ffb94a',4)
            if m.turn in ('右转','双闪'): box(630,147,13,8,'#ffb94a',4)
        # Wiring / energy flow.
        line(311,229,311,276,'#6eaa83',1)
        line(311,276,261,276,'#6eaa83',1)
        if m.key and m.current > 0:
            for i in range(3):
                f=(self.clock*.6+i/3)%1
                p.setBrush(QColor(GREEN));p.setPen(Qt.PenStyle.NoPen)
                p.drawEllipse(QPointF(311-50*f,276),3,3)
        # Component callouts stay outside vehicle silhouette.
        line(314,181,349,119,'#577968');line(349,119,430,119,'#577968')
        text(350,105,'电池 / BMS',11,'#b9cfc3');text(350,136,f'{m.soc}% · {m.remaining} km',10)
        line(248,282,157,301,'#577968');line(157,301,73,301,'#577968')
        text(73,286,'轮毂电机',11,'#b9cfc3');text(73,320,f'{m.current:.1f} A',10)
        line(657,114,718,78,'#577968');line(718,78,832,78,'#577968')
        text(718,62,'大灯 / 示廓灯',11,'#b9cfc3')
        text(44,31,'VEHICLE DIGITAL BENCH',10,'#729087')
        text(688,31,'● KEY ON' if m.key else '○ KEY OFF',10,GREEN if m.key else '#7c9695',True)
        text(44,400,'点击部件可操作  ·  车轮转速、灯光与电流实时联动',10)
        text(750,400,'← REVERSE' if m.reverse else 'FORWARD →',10,'#dcba79' if m.reverse else '#8da798')
        p.end()


class Window(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle('ZXH Vehicle Lab · 电动车模拟上位机')
        self.resize(1380,900)
        self.setMinimumSize(1060,740)
        self.model = Simulation()
        self.worker = None
        self.connected = False
        self.tx_count = self.rx_count = self.error_count = 0
        self.records = deque(maxlen=5000)
        self.last_tick = time.monotonic()
        self.settings = QSettings('ZXH','VehicleLab')
        root = QWidget(); root.setObjectName('root'); self.setCentralWidget(root)
        outer = QVBoxLayout(root); outer.setContentsMargins(22,18,22,14); outer.setSpacing(14)
        header = QHBoxLayout()
        titles = QVBoxLayout(); titles.setSpacing(2)
        titles.addWidget(label('ZXH  /  Vehicle Lab','title'))
        titles.addWidget(label('电动车可视化联调台    ·    神龙锂电 2 号协议','muted'))
        header.addLayout(titles); header.addStretch()
        self.link_label=label('● 离线预览'); self.link_label.setStyleSheet('color:#a9f078; padding:8px;')
        header.addWidget(self.link_label); outer.addLayout(header)
        connection, cl = panel(); row=QHBoxLayout()
        row.addWidget(label('串口连接','section'))
        self.port=QComboBox(); self.port.setMinimumWidth(255); self.port.setEditable(True)
        self.refresh=QPushButton('刷新');self.refresh.clicked.connect(self.refresh_ports)
        self.connect_button=QPushButton('连接仪表');self.connect_button.setObjectName('primary');self.connect_button.clicked.connect(self.toggle_connection)
        self.pause=QCheckBox('暂停发送')
        row.addWidget(self.port);row.addWidget(self.refresh);row.addWidget(label('9600 · 8N2','muted'));row.addStretch();row.addWidget(self.pause);row.addWidget(self.connect_button)
        cl.addLayout(row);outer.addWidget(connection)
        splitter=QSplitter(Qt.Orientation.Horizontal)
        left=QWidget();ll=QVBoxLayout(left);ll.setContentsMargins(0,0,0,0);ll.setSpacing(12)
        scene_panel,sl=panel();scene_head=QHBoxLayout();scene_head.addWidget(label('01   整车实时状态','section'));scene_head.addStretch()
        self.component=label('车辆模拟 · 未连接硬件','muted');scene_head.addWidget(self.component);sl.addLayout(scene_head)
        self.scene=VehicleScene(self.model,self.component_click);sl.addWidget(self.scene,1)
        self.note=label('先打开钥匙体验动画；连接串口后，开启钥匙才发送控制器反馈。','muted');self.note.setWordWrap(True);sl.addWidget(self.note)
        ll.addWidget(scene_panel,1)
        metrics=QHBoxLayout();self.metrics={}
        for key,title,unit in [('speed','车速','km/h'),('current','运行电流','A'),('soc','电池电量','%'),('assist','仪表助力','等级')]:
            card,l=panel();l.addWidget(label(title,'muted'));value=label('0','number');l.addWidget(value);l.addWidget(label(unit,'muted'));metrics.addWidget(card);self.metrics[key]=value
        ll.addLayout(metrics)
        self.tabs=QTabWidget();self.tabs.setMaximumHeight(230);self.tabs.setMinimumHeight(170)
        log_page=QWidget();log_layout=QVBoxLayout(log_page);log_layout.setContentsMargins(0,8,0,0)
        log_top=QHBoxLayout();self.counts=label('TX 0   /   RX 0   /   ERROR 0','muted');log_top.addWidget(self.counts);log_top.addStretch()
        export=QPushButton('导出 CSV');export.clicked.connect(self.export_log);clear=QPushButton('清空');clear.clicked.connect(self.clear_log);log_top.addWidget(export);log_top.addWidget(clear);log_layout.addLayout(log_top)
        self.log=QTableWidget(0,4);self.log.setHorizontalHeaderLabels(['时间','方向','HEX 报文','解析 / 事件'])
        self.log.horizontalHeader().setSectionResizeMode(0,QHeaderView.ResizeMode.ResizeToContents)
        self.log.horizontalHeader().setSectionResizeMode(1,QHeaderView.ResizeMode.ResizeToContents)
        self.log.horizontalHeader().setSectionResizeMode(2,QHeaderView.ResizeMode.ResizeToContents)
        self.log.horizontalHeader().setSectionResizeMode(3,QHeaderView.ResizeMode.Stretch)
        self.log.verticalHeader().hide();self.log.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers);self.log.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.log.setAlternatingRowColors(True);log_layout.addWidget(self.log);self.tabs.addTab(log_page,'收发监视')
        details=QWidget();dl=QVBoxLayout(details);self.command_details=label('等待仪表命令。连接后显示原始参数及最后有效报文时间。');self.command_details.setWordWrap(True);dl.addWidget(self.command_details);dl.addStretch();self.tabs.addTab(details,'仪表参数')
        support=QWidget();su=QVBoxLayout(support);support_text=label('协议支持：速度（轮周时间）、电流、电量、剩余里程、刹车、充电及故障反馈；接收仪表灯光、助力、控制器开关和倒车命令。\n\n仅动画：钥匙电源、示廓灯、左右转向灯。钥匙关闭会停止上位机发帧，不会切断真实仪表供电。倒车没有独立控制器反馈位。\n\n数据口：仪表 RX PC05 / TX PC04（本工程默认配置，接线前核对 PCB）；3.3V TTL，收发交叉并共地。PA12 是原调试日志口。')
        support_text.setWordWrap(True);su.addWidget(support_text);su.addStretch();self.tabs.addTab(support,'功能与接线')
        ll.addWidget(self.tabs)
        right=QScrollArea();right.setWidgetResizable(True);right.setMinimumWidth(310);right.setMaximumWidth(410)
        controls=QWidget();rl=QVBoxLayout(controls);rl.setContentsMargins(3,0,4,0);rl.setSpacing(12)
        power,pl=panel();pl.addWidget(label('02   驾驶与灯光','section'))
        self.key_button=QPushButton('钥匙 OFF · 点击上电');self.key_button.setCheckable(True);self.key_button.toggled.connect(self.key_changed);pl.addWidget(self.key_button)
        self.follow=QCheckBox('跟随真实仪表指令');self.follow.setChecked(True);self.follow.toggled.connect(self.follow_changed);pl.addWidget(self.follow)
        hint=label('联机时跟随仪表开关、灯光、倒车；离线可直接操作。','muted');hint.setWordWrap(True);pl.addWidget(hint)
        self.light_button=self.toggle_button('大灯',lambda v:setattr(self.model,'light',v))
        self.position_button=self.toggle_button('示廓灯 · 动画',lambda v:setattr(self.model,'position',v))
        lights=QHBoxLayout();lights.addWidget(self.light_button);lights.addWidget(self.position_button);pl.addLayout(lights)
        self.brake_button=self.toggle_button('刹车',lambda v:setattr(self.model,'brake',v))
        self.reverse_button=self.toggle_button('倒车 · 动画',lambda v:setattr(self.model,'reverse',v))
        rr=QHBoxLayout();rr.addWidget(self.brake_button);rr.addWidget(self.reverse_button);pl.addLayout(rr)
        tr=QHBoxLayout();tr.addWidget(label('转向灯'));self.turn=QComboBox();self.turn.addItems(['关闭','左转','右转','双闪']);self.turn.currentTextChanged.connect(lambda t:setattr(self.model,'turn',t));tr.addWidget(self.turn,1);pl.addLayout(tr)
        rl.addWidget(power)
        drive,dr=panel();dr.addWidget(label('03   动力与电池','section'))
        self.throttle,self.throttle_value=self.slider(dr,'转把',0,100,0,' %',lambda v:setattr(self.model,'throttle',v))
        self.auto=QCheckBox('根据转把模拟加减速');self.auto.setChecked(True);self.auto.toggled.connect(self.auto_changed);dr.addWidget(self.auto)
        self.speed_slider,self.speed_value=self.slider(dr,'目标车速',0,100,0,' km/h',lambda v:setattr(self.model,'target_speed',v));self.speed_slider.setEnabled(False)
        self.soc_slider,_=self.slider(dr,'电量',0,100,80,' %',lambda v:setattr(self.model,'soc',v))
        rp=QHBoxLayout();rp.addWidget(label('剩余里程'));self.remaining=QSpinBox();self.remaining.setRange(0,65535);self.remaining.setValue(45);self.remaining.setSuffix(' km');self.remaining.valueChanged.connect(lambda v:setattr(self.model,'remaining',v));rp.addWidget(self.remaining);dr.addLayout(rp)
        rp=QHBoxLayout();rp.addWidget(label('轮径'));self.diameter=QDoubleSpinBox();self.diameter.setRange(6,40);self.diameter.setDecimals(1);self.diameter.setValue(16);self.diameter.setSuffix(' 英寸');self.diameter.valueChanged.connect(lambda v:setattr(self.model,'diameter',v));rp.addWidget(self.diameter);dr.addLayout(rp)
        self.charge=QCheckBox('充电状态');self.charge.toggled.connect(lambda v:setattr(self.model,'charging',v));dr.addWidget(self.charge)
        rl.addWidget(drive)
        fault,fl=panel();fl.addWidget(label('04   故障注入','section'));self.fault_checks={}
        for key,title in [('hall','电机霍尔故障'),('throttle','转把故障'),('controller','控制器故障'),('low','欠压保护'),('phase','电机缺相')]:
            check=QCheckBox(title);check.toggled.connect(lambda v,k=key:self.set_fault(k,v));fl.addWidget(check);self.fault_checks[key]=check
        reset=QPushButton('恢复车辆默认状态');reset.clicked.connect(self.reset_model);fl.addWidget(reset);rl.addWidget(fault);rl.addStretch();right.setWidget(controls)
        splitter.addWidget(left);splitter.addWidget(right);splitter.setSizes([970,340]);splitter.setChildrenCollapsible(False);outer.addWidget(splitter,1)
        footer=QHBoxLayout();footer.addWidget(label('CONTROLLER SIMULATOR   /   v1.0','muted'));footer.addStretch();self.footer_status=label('离线预览可直接使用；串口连接不会自动发帧。','muted');footer.addWidget(self.footer_status);outer.addLayout(footer)
        self.refresh_ports()
        self.timer=QTimer(self);self.timer.timeout.connect(self.tick);self.timer.start(30)

    def toggle_button(self,text,callback):
        b=QPushButton(text);b.setCheckable(True);b.toggled.connect(callback);return b

    def slider(self,layout,title,lo,hi,value,suffix,callback):
        row=QHBoxLayout();row.addWidget(label(title));row.addStretch();v=label(f'{value}{suffix}','muted');row.addWidget(v);layout.addLayout(row)
        s=QSlider(Qt.Orientation.Horizontal);s.setRange(lo,hi);s.setValue(value)
        s.valueChanged.connect(lambda n:(v.setText(f'{n}{suffix}'),callback(n)));layout.addWidget(s);return s,v

    def refresh_ports(self):
        previous=self.port.currentData() or self.port.currentText() or self.settings.value('port','')
        self.port.clear()
        for item in list_ports.comports(): self.port.addItem(f'{item.device}  ·  {item.description}',item.device)
        index=self.port.findData(previous)
        if index>=0:self.port.setCurrentIndex(index)
        elif previous:self.port.setEditText(str(previous))
        elif not self.port.count():self.port.setEditText('')
        self.port.setPlaceholderText('选择或输入 COM 口')

    def toggle_connection(self):
        if self.worker:
            self.disconnect_port();return
        port=self.port.currentData() or self.port.currentText().strip().split(' ')[0]
        if not port:
            QMessageBox.information(self,'串口连接','请插入 USB 转串口，刷新列表或输入 COM 口。');return
        self.key_button.setChecked(False)
        self.model.command=None;self.model.last_command=-1e10
        self.settings.setValue('port',port)
        self.worker=SerialWorker(port,self);self.worker.received.connect(self.received);self.worker.transmitted.connect(self.transmitted)
        self.worker.failed.connect(self.serial_error);self.worker.opened.connect(self.serial_opened);self.worker.diagnostics.connect(self.serial_diagnostics)
        self.worker.finished.connect(self.worker_finished);self.connect_button.setText('正在连接…');self.connect_button.setEnabled(False)
        self.port.setEnabled(False);self.refresh.setEnabled(False);self.worker.start()

    def serial_opened(self):
        self.connected=True;self.connect_button.setEnabled(True);self.connect_button.setText('断开串口');self.add_log('SYS',b'','串口已打开：9600 8N2；等待仪表命令')

    def disconnect_port(self):
        if self.worker:
            self.connect_button.setEnabled(False);self.connect_button.setText('正在断开…');self.worker.stop()

    def worker_finished(self):
        old=self.worker
        self.worker=None;self.connected=False
        if old:old.deleteLater()
        self.connect_button.setEnabled(True);self.connect_button.setText('连接仪表');self.port.setEnabled(True);self.refresh.setEnabled(True)
        self.model.command=None;self.model.last_command=-1e10;self.model.cruise=False
        self.add_log('SYS',b'','串口已关闭，进入离线预览')

    def serial_error(self,message):
        self.error_count+=1;self.add_log('ERR',b'',message);self.footer_status.setText('串口异常：'+message)
        QMessageBox.warning(self,'串口异常',message)

    def serial_diagnostics(self,count):
        self.error_count=count;self.add_log('ERR',b'',f'接收噪声/错误帧累计：{count}')

    def received(self,raw,cmd):
        self.rx_count+=1;self.model.command_received(cmd,time.monotonic())
        self.add_log('RX',raw,f'仪表→控制器  灯={int(cmd.light)}  开关={int(cmd.power)}  助力={cmd.assist}  倒车={int(cmd.reverse)}')
        self.command_details.setText(f'驱动方式 {cmd.drive}   /   助力等级 {cmd.assist}   /   开关 {int(cmd.power)}   /   灯光 {int(cmd.light)}   /   倒车 {int(cmd.reverse)}\n轮径 {cmd.diameter_tenth_inch/10:.1f} 英寸   /   测速磁钢 {cmd.magnets}   /   内测速磁钢 {cmd.inner_magnets}\n限速 {cmd.speed_limit} km/h   /   限流 {cmd.current_limit} A   /   欠压 {cmd.undervoltage_tenth_v/10:.1f} V\n转把 PWM {cmd.throttle_pwm/10:.1f}%   /   助力 PWM {cmd.assist_pwm if cmd.assist_pwm is not None else "无（20字节）"}\n接收时间 {datetime.now():%H:%M:%S}  ·  轮径以右侧参数为准，请与仪表一致')

    def transmitted(self,raw):
        self.tx_count+=1;self.add_log('TX',raw,f'控制器→仪表  {self.model.speed:.1f} km/h  {self.model.current:.1f} A  SOC {self.model.soc}%')

    def add_log(self,direction,raw,summary):
        record=(datetime.now().isoformat(timespec='milliseconds'),direction,raw.hex(' ').upper(),summary)
        self.records.append(record)
        if self.log.rowCount()>=300:self.log.removeRow(0)
        row=self.log.rowCount();self.log.insertRow(row)
        for col,value in enumerate((record[0][11:],*record[1:])):
            item=QTableWidgetItem(value)
            if col==1:item.setForeground(QColor(GREEN if direction=='TX' else '#8ac9ff' if direction=='RX' else '#d9ab7e'))
            self.log.setItem(row,col,item)
        self.log.scrollToBottom()

    def clear_log(self):
        self.records.clear();self.log.setRowCount(0);self.tx_count=self.rx_count=self.error_count=0

    def export_log(self):
        path,_=QFileDialog.getSaveFileName(self,'导出收发日志',f'车辆联调-{datetime.now():%Y%m%d-%H%M%S}.csv','CSV (*.csv)')
        if path:
            try:
                with open(path,'w',encoding='utf-8-sig',newline='') as f:
                    w=csv.writer(f);w.writerow(['时间','方向','HEX','解析/事件']);w.writerows(self.records)
                self.footer_status.setText('日志已保存：'+path)
            except OSError as e:QMessageBox.warning(self,'导出失败',str(e))

    def key_changed(self,value):
        self.model.key=value
        if not value:self.model.cruise=False
        self.key_button.setText('钥匙 ON · 点击下电' if value else '钥匙 OFF · 点击上电')
        if self.worker and not value:self.worker.set_frame(None)

    def follow_changed(self,value):
        self.model.follow_meter=value;self.model.cruise=False;self.model.previous_toggle=False

    def auto_changed(self,value):
        self.model.automatic=value;self.speed_slider.setEnabled(not value);self.throttle.setEnabled(value)

    def set_fault(self,key,value):
        if value:self.model.faults.add(key)
        else:self.model.faults.discard(key)

    def component_click(self,name):
        self.component.setText('当前部件：'+name)
        if name=='大灯' and self.light_button.isEnabled():self.light_button.toggle()
        elif name=='刹车':self.brake_button.toggle()
        elif name=='电池':self.tabs.setCurrentIndex(1)

    def reset_model(self):
        self.key_button.setChecked(False);self.brake_button.setChecked(False);self.light_button.setChecked(False);self.position_button.setChecked(False);self.reverse_button.setChecked(False)
        self.turn.setCurrentIndex(0);self.throttle.setValue(0);self.speed_slider.setValue(0);self.soc_slider.setValue(80);self.remaining.setValue(45);self.charge.setChecked(False)
        self.model.speed=self.model.current=0;self.model.cruise=False
        for c in self.fault_checks.values():c.setChecked(False)

    def tick(self):
        now=time.monotonic();dt=now-self.last_tick;self.last_tick=now
        m=self.model;m.step(dt,now,self.connected)
        if self.worker:self.worker.set_frame(m.feedback() if m.key and not self.pause.isChecked() else None)
        self.scene.clock=now;self.scene.phase+=m.speed*dt*( -3 if m.reverse else 3);self.scene.update()
        self.metrics['speed'].setText(f'{m.speed:.1f}');self.metrics['current'].setText(f'{m.current:.1f}');self.metrics['soc'].setText(str(m.soc))
        online=self.connected and m.meter_online(now)
        self.metrics['assist'].setText(str(m.command.assist) if online else '—')
        follow=self.connected and m.follow_meter
        self.light_button.setEnabled(not follow);self.reverse_button.setEnabled(not follow)
        if follow:
            for button,value in [(self.light_button,m.light),(self.reverse_button,m.reverse)]:
                button.blockSignals(True);button.setChecked(value);button.blockSignals(False)
        status='● 仪表在线 · 双向通信' if online else '● 串口已打开 · 等待仪表' if self.connected else '○ 离线预览'
        self.link_label.setText(status)
        self.counts.setText(f'TX {self.tx_count}   /   RX {self.rx_count}   /   ERROR {self.error_count}')
        self.footer_status.setText(('发送已暂停' if self.pause.isChecked() else '正在发送控制器反馈 · 120 ms' if self.connected and m.key else '钥匙关闭 · 停止发送' if self.connected else '离线预览 · 动画和参数不发送到硬件') + ('  |  仪表命令超时，动力已停止' if follow and not online else ''))

    def closeEvent(self,event):
        self.timer.stop()
        if self.worker:
            self.worker.stop()
            if not self.worker.wait(1500):
                self.timer.start(30);event.ignore();return
        event.accept()


def main():
    app=QApplication(sys.argv);app.setStyle('Fusion');app.setStyleSheet(STYLE)
    win=Window();win.show()
    if '--screenshot' in sys.argv:
        path=Path(sys.argv[sys.argv.index('--screenshot')+1])
        def shot():
            win.key_button.setChecked(True);win.light_button.setChecked(True);win.position_button.setChecked(True);win.throttle.setValue(48);win.model.speed=28.8
            win.tick();win.grab().save(str(path));app.quit()
        QTimer.singleShot(350,shot)
    sys.exit(app.exec())


if __name__=='__main__':main()
