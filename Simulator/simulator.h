#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <QtWidgets>
#include <QUdpSocket>
#include <stdint.h>

//#define PRI_ENABLE

extern "C" {
void sim_rx_handle(uint8_t pri, uint8_t addr, QByteArray &data);
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
	explicit Simulator(QWidget *parent = 0);
	~Simulator();
	void transmit(uint8_t pri, uint8_t addr, QByteArray &data);
	void received(uint8_t addr, void *ptr, int len);
	void *memAlloc(int size);
	void memFree(void *ptr);

signals:
	void scrollTransmitLog();
	void scrollTXLog();
	void scrollRXLog();
	void updateMem();
	void error(QString);

public slots:
	void write();
	void read();

private slots:
	void updateAddr();
	void updateMemList();
	void txScroll();
	void rxScroll();
	void errorMessage(QString str);

private:
	static QString dataString(const char *ptr, uint8_t len);

	QMutex mutexMem;
	QMap<void *, int> mapMem;
	QUdpSocket socket, gSocket;
#ifdef PRI_ENABLE
	QCheckBox *pri;
#endif
	QCheckBox *memClear;
	QLineEdit *input, *macAddr, *netAddr, *destAddr;
	QListWidget *tx, *rx, *mem;
	QListWidget *transmitLog, *receivedLog;
	QPushButton *send;
};

extern class Simulator *sim;

#endif // SIMULATOR_H
