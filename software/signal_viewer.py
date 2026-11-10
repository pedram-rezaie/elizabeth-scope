import numpy as np
from PySide6.QtWidgets import QWidget, QVBoxLayout
import pyqtgraph as pg

class SignalViewer(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Signal viewer")
        self.resize(1200, 600)

        layout = QVBoxLayout(self)
        self.plot = pg.PlotWidget()
        layout.addWidget(self.plot)

        self.plot.setLabel('bottom', 'sample index')
        self.plot.setLabel('left', 'ADC number')
        self.curve = self.plot.plot()

    def update_data(self, data):
        self.samples = np.frombuffer(data, dtype=np.uint8)
        self.t = np.arange(len(self.samples))
        self.curve.setData(self.t, self.samples)
