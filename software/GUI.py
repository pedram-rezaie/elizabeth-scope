import numpy as np
import pyqtgraph as pg
from PySide6.QtWidgets import (QMainWindow, QWidget, QVBoxLayout,
                               QHBoxLayout, QPushButton, QSpinBox, QLineEdit, QLabel,QComboBox, QToolBar,
                               QScrollArea, QGroupBox, QMessageBox)
import control_functions as f
from signal_viewer import SignalViewer
class MainWindow(QMainWindow):
    def __init__(self, dev):


        super().__init__()
        self.dev = dev
        self.viewer = SignalViewer()
        self.adc_data = bytearray(8192)
        self.setWindowTitle("Oscilloscope")
        self.resize(1200, 800)
        central = QWidget()
        self.setCentralWidget(central)
        #  main_layout = QHBoxLayout(central) = main_layout = QVBoxLayout(); central.setLayout(main_layout);
        # we weill split the main in 2 than add row of widgets to left to fill in the functions
        main_layout = QHBoxLayout(central)
        left_widget = QWidget()                 # holds the left layout
        left = QVBoxLayout(left_widget)
        left_scroll = QScrollArea()             # the scroll thingy
        left_scroll.setWidgetResizable(True)    # left_widget stretches to the scroll area's width
        left_scroll.setWidget(left_widget)
        right = QVBoxLayout()
        main_layout.addWidget(left_scroll,10)
        main_layout.addLayout(right,10)
        toolbar = QToolBar("tools")
        self.addToolBar(toolbar)
        toolbar.addAction("signal viewer", self.open_viewer)

        #--- mux set area begin

        self.mux_update = QSpinBox()
        self.mux_level = QSpinBox()
        self.mux_update.setRange(0,1)
        self.mux_level.setRange(1,8)
        self.mux_update.setValue(0)
        self.mux_level.setValue(1)
        self.mux_set_button = QPushButton("mux set")

        mux_widget = QGroupBox("mux")
        mux_columns = QHBoxLayout(mux_widget)
        mux_columns.addWidget(self.mux_set_button)
        mux_columns.addSpacing(20)
        mux_columns.addWidget(QLabel("enable: "))
        mux_columns.addWidget(self.mux_update)
        mux_columns.addSpacing(20)
        mux_columns.addWidget(QLabel("gain (x): "))
        mux_columns.addWidget(self.mux_level)
        mux_columns.addStretch()
        left.addWidget(mux_widget)

        self.mux_set_button.clicked.connect(self.mux_apply)

        #------- mux set area end----------------
        #-------dac1 set area start -------------
        self.dac1_update = QSpinBox()
        self.dac1_triangle = QSpinBox()
        self.dac1_update.setRange(0, 1)
        self.dac1_triangle.setRange(0, 1)
        self.dac1_update.setValue(0)
        self.dac1_triangle.setValue(0)

        self.dac1_freq = QLineEdit()
        self.dac1_amp = QLineEdit()
        self.dac1_dc = QLineEdit()
        self.dac1_freq.setText("1000")
        self.dac1_amp.setText("1")
        self.dac1_dc.setText("1.5")
        self.dac1_freq.setToolTip("max 30000")
        self.dac1_amp.setToolTip("max 3")
        self.dac1_dc.setToolTip("max 3")

        self.dac1_set_button = QPushButton("dac1 set")
        self.dac1_off_button = QPushButton("dac1 off")
        self.dac1_off_button.clicked.connect(self.dac1_off_apply)

        dac1_widget = QGroupBox("DAC1")
        dac1_box = QVBoxLayout(dac1_widget)                  # box: note on top, row below
        dac1_note = QLabel("amplitude here is just from 0 to top the actual amplitude is half of that")
        dac1_note.setWordWrap(True)                          # wraps instead of widening the box
        dac1_box.addWidget(dac1_note)
        dac1_columns = QHBoxLayout()                         # no parent widget now, it goes into dac1_box
        dac1_box.addLayout(dac1_columns)
        dac1_columns.addWidget(self.dac1_set_button)
        dac1_columns.addWidget(self.dac1_off_button)
        dac1_columns.addSpacing(20)
        dac1_columns.addWidget(QLabel("enable: "))
        dac1_columns.addWidget(self.dac1_update)
        dac1_columns.addSpacing(20)
        dac1_columns.addWidget(QLabel("triangle on: "))
        dac1_columns.addWidget(self.dac1_triangle)
        dac1_columns.addSpacing(20)
        dac1_columns.addWidget(QLabel("f (Hz): "))
        dac1_columns.addWidget(self.dac1_freq)
        dac1_columns.addSpacing(20)
        dac1_columns.addWidget(QLabel("amp (V p-p): "))
        dac1_columns.addWidget(self.dac1_amp)
        dac1_columns.addSpacing(20)
        dac1_columns.addWidget(QLabel("dc (V): "))
        dac1_columns.addWidget(self.dac1_dc)
        dac1_columns.addStretch()
        left.addWidget(dac1_widget)
        self.dac1_set_button.clicked.connect(self.dac1_apply)

        #-------dac1 set area end -------------
        #-------dac2 set start area------------
        self.dac2_shape = QComboBox()
        self.dac2_shape.addItems(["sine", "square", "triangle", "sawtooth"])
        self.dac2_update = QSpinBox()
        self.dac2_update.setRange(0,1)
        self.dac2_table_update = QSpinBox()
        self.dac2_table_update.setRange(0,1)
        self.dac2_update.setValue(0)
        self.dac2_table_update.setValue(0)
        self.dac2_freq = QLineEdit()
        self.dac2_amp = QLineEdit()
        self.dac2_dc = QLineEdit()
        self.dac2_freq.setText("1000")
        self.dac2_amp.setText("1")
        self.dac2_dc.setText("1.5")
        self.dac2_freq.setToolTip("max 30000")
        self.dac2_amp.setToolTip("max 1.5")
        self.dac2_dc.setToolTip("max 3")
        self.dac2_set_button = QPushButton("dac2 set")
        self.dac2_set_button.clicked.connect(self.dac2_apply)
        self.dac2_off_button = QPushButton("dac2 off")
        self.dac2_off_button.clicked.connect(self.dac2_off_apply)
        self.dac2_columns = QGroupBox("DAC2")
        self.dac2_layout = QHBoxLayout(self.dac2_columns)
        self.dac2_layout.addWidget(self.dac2_set_button)
        self.dac2_layout.addWidget(self.dac2_off_button)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("shape:"))
        self.dac2_layout.addWidget(self.dac2_shape)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("amp (V peak):"))
        self.dac2_layout.addWidget(self.dac2_amp)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("f (Hz):"))
        self.dac2_layout.addWidget(self.dac2_freq)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("dc (V):"))
        self.dac2_layout.addWidget(self.dac2_dc)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("enable:"))
        self.dac2_layout.addWidget(self.dac2_update)
        self.dac2_layout.addSpacing(10)
        self.dac2_layout.addWidget(QLabel("new table:"))
        self.dac2_layout.addWidget(self.dac2_table_update)
        self.dac2_layout.addSpacing(10)
        left.addWidget(self.dac2_columns)

        #-------dac2 set end area------------
        #-------adc clock start--------------
        self.adc_update = QSpinBox()
        self.adc_update.setRange(0,1)
        self.adc_update.setValue(0)
        self.adc_arr = QLineEdit()
        self.adc_ccr2 = QLineEdit()
        self.adc_arr.setText("9")                        # 168 MHz / 10 = 16.8 MHz
        self.adc_ccr2.setText("5")
        self.adc_arr.setToolTip("f = 168 MHz / (arr + 1)")
        self.adc_ccr2.setToolTip("0..arr, above arr the DMA never fires and the capture hangs")
        self.adc_speed = QComboBox()                     # drop down, adc_clock_set turns the string into the OSPEEDR bits
        self.adc_speed.addItems(["low (2 MHz)", "medium (25 MHz)", "fast (50 MHz)", "high (100 MHz)"])
        self.adc_speed.setCurrentIndex(3)                # what tim1_init sets
        self.adc_set_button = QPushButton("clock set")
        self.adc_set_button.clicked.connect(self.adc_clock_apply)
        adc_widget = QGroupBox("ADC clock")
        adc_columns = QHBoxLayout(adc_widget)
        adc_columns.addWidget(self.adc_set_button)
        adc_columns.addSpacing(20)
        adc_columns.addWidget(QLabel("enable: "))
        adc_columns.addWidget(self.adc_update)
        adc_columns.addSpacing(20)
        adc_columns.addWidget(QLabel("ARR: "))
        adc_columns.addWidget(self.adc_arr)
        adc_columns.addSpacing(20)
        adc_columns.addWidget(QLabel("CCR2: "))
        adc_columns.addWidget(self.adc_ccr2)
        adc_columns.addSpacing(20)
        adc_columns.addWidget(QLabel("PA8 speed: "))
        adc_columns.addWidget(self.adc_speed)
        adc_columns.addStretch()
        left.addWidget(adc_widget)
        #-------adc clock end----------------
        #-------capture command--------------
        self.capture_mode = QComboBox()
        self.capture_mode.addItems(["burst capture", "continuous capture", "true continuous", "turn off"])
        self.command_set_button = QPushButton("capture")
        self.command_set_button.clicked.connect(self.command_apply)
        self.start_capture = QSpinBox()
        self.start_capture.setRange(0,1)
        self.start_capture.setValue(0)
        self.command_columns = QGroupBox("capture")
        self.command_layout = QHBoxLayout(self.command_columns)
        self.command_layout.addWidget(self.command_set_button)
        self.command_layout.addWidget(QLabel("mode:"))
        self.command_layout.addWidget(self.capture_mode)
        self.command_layout.addWidget(QLabel("enable:"))
        self.command_layout.addWidget(self.start_capture)
        left.addWidget(self.command_columns)
        #-------capture command--------------
        left.addStretch()

        #-------signal preview start-------------
        self.preview = pg.PlotWidget()
        self.preview.setLabel('bottom', 'sample index')
        self.preview.setLabel('left', 'ADC number')
        self.preview.setYRange(0, 255)                       # uint8 samples, fixed y axis
        self.preview.showGrid(x=True, y=True)
        self.preview.setMouseEnabled(x=False, y=False)       # just a preview, zooming is the viewer's job
        self.preview_curve = self.preview.plot()             # empty curve, data comes later
        self.preview_curve.setDownsampling(auto=True, method='peak')
        self.preview_curve.setClipToView(True)
        right.addWidget(self.preview)
        #-------signal preview end---------------

    def mux_apply(self):
        f.clear_enables()
        f.mux_set(self.mux_update.value(), self.mux_level.value())
        self.send_command()

    def dac1_apply(self):
        f.clear_enables()
        dc = f.volts_to_code(float(self.dac1_dc.text()))
        amp = f.volts_to_code(float(self.dac1_amp.text()))
        f.dac1_set(dc,
        self.dac1_triangle.value(),
        int(self.dac1_freq.text() or 0),
        amp,
        0,                              # turn off has its own button now
        self.dac1_update.value())
        self.send_command()

    def dac1_off_apply(self):
        f.clear_enables()
        f.dac1_turn_off()
        self.send_command()
        f.command[8] &= 0xFC            # update and turn off back to 0

    def dac2_apply(self):
        f.clear_enables()
        amp = f.volts_to_code(float(self.dac2_amp.text()))
        dc = f.volts_to_code(float(self.dac2_dc.text()))
        table, l, arr = f.dac2_table_generator(amp, int(self.dac2_freq.text()), dc,
                                            self.dac2_shape.currentText())
        f.dac2_activate(table, l, arr, self.dac2_update.value(),
                        0, self.dac2_table_update.value())
        self.send_command()

    def dac2_off_apply(self):
        f.clear_enables()
        f.dac2_turn_off()
        self.send_command()
        f.command[20] &= 0xF8           # table_update, turn off, update back to 0

    def adc_clock_apply(self):
        f.clear_enables()
        f.adc_clock_set(int(self.adc_ccr2.text()), int(self.adc_arr.text()),
                        self.adc_speed.currentText(), self.adc_update.value())
        self.send_command()

    def command_apply(self):
        mode = self.capture_mode.currentText()
        # continuous modes are placeholders, the firmware only does burst for now
        if(mode == "continuous capture" or mode == "true continuous"):
            QMessageBox.warning(self, "capture", "we dont support that yet")
            return
        f.clear_enables()
        f.capture_command(int(self.start_capture.text()), mode)
        self.send_command()

    def send_command(self):
        self.adc_data = f.send_command(self.dev)
        if not isinstance(self.adc_data, int):
            self.adc_data = f.bit_reverse(self.adc_data)
            self.viewer.update_data(self.adc_data)
            self.preview_update(self.adc_data)

    def preview_update(self, data):
        samples = np.frombuffer(data, dtype=np.uint8)        # 122624 bytes -> 122624 samples
        n = np.arange(len(samples))                          # x axis is the sample index
        self.preview_curve.setData(n, samples)

    def open_viewer(self):

        self.viewer.show()
