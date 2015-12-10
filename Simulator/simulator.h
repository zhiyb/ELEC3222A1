#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QtWidgets>
#include <QUdpSocket>
#include <stdint.h>

extern "C" {
void sim_rx_handle(uint8_t addr, char *ptr, int len);
void sim_start();
void sim_end();
}

struct addr_t {
	uint8_t mac, net;
};

extern addr_t addr;

class Simulator : public QWidget
{
	Q_OBJECT
public:
	explicit Simulator(uint8_t address, QWidget *parent = 0);
	~Simulator();
	void transmit(uint8_t addr, void *ptr, int len);
	void received(uint8_t addr, void *ptr, int len);

signals:

public slots:
	void write();
	void read();

private:
	QString dataString(char *ptr, uint8_t len);

	QUdpSocket *socket;
	QCheckBox *pri;
	QLineEdit *input, *address;
	QListWidget *transmitLog, *receivedLog;
	QPushButton *send;
};

extern class Simulator *sim;

#endif // SIMULATOR_H
