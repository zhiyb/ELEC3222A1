#include "mainwindow.h"
#include "simulator.h"

MainWindow::MainWindow(uint8_t address, QWidget *parent)
	: QMainWindow(parent)
{
	Simulator *s;
	setCentralWidget(s = new Simulator(address, this));
	setWindowTitle(QString("Simulator, mac: %1, net: %2").arg(addr.mac, 0, 16).arg(addr.net, 0, 16));
}

MainWindow::~MainWindow()
{

}
