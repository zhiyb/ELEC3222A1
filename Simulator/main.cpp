#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	if (argc != 2)
		return 1;
	MainWindow w(QString(argv[1]).toUInt(0, 16));
	w.show();

	return a.exec();
}
