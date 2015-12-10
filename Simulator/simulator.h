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
	friend class TXTask;
	friend class RXTask;
public:
	explicit Simulator(uint8_t address, QWidget *parent = 0);
	~Simulator();
	void transmit(uint8_t addr, void *ptr, int len);
	void received(uint8_t addr, void *ptr, int len);
	void *memAlloc(int size);
	void memFree(void *ptr);

signals:

public slots:
	void write();
	void read();

private:
	static QString dataString(char *ptr, uint8_t len);
	void updateMemList();

	QMap<void *, int> mapMem;
	QUdpSocket *socket;
	QCheckBox *pri;
	QLineEdit *input, *address;
	QListWidget *tx, *rx, *mem;
	QListWidget *transmitLog, *receivedLog;
	QPushButton *send;
};

extern class Simulator *sim;

#endif // SIMULATOR_H
