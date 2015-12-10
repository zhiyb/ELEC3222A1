#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	if (argc != 2)
		return 1;
	MainWindow w(atoi(argv[1]));
	w.show();

	return a.exec();
}
