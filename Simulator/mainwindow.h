#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <stdint.h>

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(uint8_t address, QWidget *parent = 0);
	~MainWindow();
};

#endif // MAINWINDOW_H
