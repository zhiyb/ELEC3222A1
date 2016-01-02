#include "simulator.h"
#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	Simulator s;
	s.show();

	return a.exec();
}
