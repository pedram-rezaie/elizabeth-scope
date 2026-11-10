import sys
import libusb_package
from PySide6.QtWidgets import QApplication
from GUI import MainWindow

dev = libusb_package.find(idVendor=0x1209, idProduct=0x0001)
if dev is None:
    raise RuntimeError("scope not found")
dev.set_configuration()

app = QApplication(sys.argv)
window = MainWindow(dev)
window.show()
sys.exit(app.exec())
