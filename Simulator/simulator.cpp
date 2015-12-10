#include "simulator.h"
#include "../llc_layer.h"
#include "../mac_layer.h"

#define BASE	30000

addr_t addr;
class Simulator *sim;

class TXTask: public QRunnable
{
public:
	TXTask(uint8_t pri, uint8_t addr, int len, char *ptr):
		QRunnable(), pri(pri), addr(addr) {data.resize(len); memcpy(data.data(), ptr, len);}

	void run() {llc_tx(pri, addr, data.size(), data.data());}

private:
	uint8_t pri;
	uint8_t addr;
	QByteArray data;
	void *param;
	QString name;
};

Simulator::Simulator(uint8_t address, QWidget *parent) : QWidget(parent)
{
	sim = this;
	socket = new QUdpSocket(this);

	QVBoxLayout *lay = new QVBoxLayout(this);
	lay->addWidget(this->address = new QLineEdit());
	lay->addWidget(pri = new QCheckBox(tr("Acknowledged")));
	lay->addWidget(input = new QLineEdit(tr("Input...")));
	lay->addWidget(send = new QPushButton(tr("Send")));

	QHBoxLayout *loLay = new QHBoxLayout;
	lay->addLayout(loLay);
	loLay->addWidget(transmitLog = new QListWidget);
	loLay->addWidget(receivedLog = new QListWidget);

	addr.mac = address;
	addr.net = address;
	socket->bind(BASE + address);

	connect(send, SIGNAL(clicked(bool)), this, SLOT(write()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(read()));

	sim_start();
}

Simulator::~Simulator()
{
	sim_end();
}

void Simulator::write()
{
	QString str(input->text());
	TXTask *task = new TXTask(pri->isChecked() ? DL_DATA_ACK : DL_UNITDATA, address->text().toUInt(0, 16), str.length(), str.toLocal8Bit().data());
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);
}

void Simulator::read()
{
	while (socket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(socket->pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		socket->readDatagram(datagram.data(), datagram.size(),
					&sender, &senderPort);

		uint8_t addr = senderPort - BASE;
		uint8_t len = datagram.size();
		receivedLog->addItem(QString("Address: %1, length %2: %3").arg(addr).arg(len).arg(dataString(datagram.data(), len)));
		sim_rx_handle(addr, datagram.data(), len);
	}
}

QString Simulator::dataString(char *ptr, uint8_t len)
{
	QString str;
	while (len--) {
		char data = *ptr++;
		if (isprint(data))
			str.append(data);
		else {
			str.append('\\');
			str.append('0' + ((data >> 6) & 0x07));
			str.append('0' + ((data >> 3) & 0x07));
			str.append('0' + ((data >> 0) & 0x07));
		}
	}
	return str;
}

void Simulator::transmit(uint8_t addr, void *ptr, int len)
{
	socket->writeDatagram((char *)ptr, len, QHostAddress::LocalHost, BASE + addr);
	socket->waitForBytesWritten();
	transmitLog->addItem(QString("Address: %1, length %2: %3").arg(addr).arg(len).arg(dataString((char *)ptr, len)));
}
