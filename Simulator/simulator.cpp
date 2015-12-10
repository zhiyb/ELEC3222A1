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

	void run();

private:
	uint8_t pri;
	uint8_t addr;
	QByteArray data;
	void *param;
	QString name;
};

class RXTask: public QRunnable
{
public:
	RXTask(): QRunnable() {}

	void run();
};

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

void Simulator::updateMemList()
{
	mem->clear();
	QMap<void *, int>::const_iterator it = mapMem.constBegin();
	while (it != mapMem.constEnd()) {
		mem->addItem(tr("0x%1: SIZE %2").arg((quint64)it.key(), 0, 16).arg(it.value()));
		it++;
	}
}

Simulator::Simulator(uint8_t address, QWidget *parent) : QWidget(parent)
{
	sim = this;
	socket = new QUdpSocket(this);

	QVBoxLayout *lay = new QVBoxLayout(this);
	lay->addWidget(this->address = new QLineEdit());
	lay->addWidget(pri = new QCheckBox(tr("Request ACK")));
	lay->addWidget(input = new QLineEdit(tr("Input...")));
	lay->addWidget(send = new QPushButton(tr("Transmit")));

	QHBoxLayout *hLay;
	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(tx = new QListWidget);
	hLay->addWidget(rx = new QListWidget);
	hLay->addWidget(mem = new QListWidget);

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(transmitLog = new QListWidget);
	hLay->addWidget(receivedLog = new QListWidget);

	addr.mac = address;
	addr.net = address;
	socket->bind(BASE + address);

	connect(send, SIGNAL(clicked(bool)), this, SLOT(write()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(read()));

	sim_start();

	RXTask *task = new RXTask;
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);
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
		receivedLog->addItem(tr("ADDR %1, LED %2: %3").arg(addr, 0, 16).arg(len).arg(dataString(datagram.data(), len)));
		receivedLog->setCurrentRow(receivedLog->count() - 1);
		sim_rx_handle(addr, datagram.data(), len);
	}
}

void Simulator::transmit(uint8_t addr, void *ptr, int len)
{
	socket->writeDatagram((char *)ptr, len, QHostAddress::LocalHost, BASE + addr);
	socket->waitForBytesWritten();
	transmitLog->addItem(tr("ADDR %1, LEN %2: %3").arg(addr, 0, 16).arg(len).arg(dataString((char *)ptr, len)));
	transmitLog->setCurrentRow(transmitLog->count() - 1);
}

void *Simulator::memAlloc(int size)
{
	void *ptr = malloc(size);
	mapMem[ptr] = size;
	updateMemList();
	return ptr;
}

void Simulator::memFree(void *ptr)
{
	free(ptr);
	mapMem.remove(ptr);
	updateMemList();
}

void TXTask::run()
{
	uint8_t ret = llc_tx(pri, addr, data.size(), data.data());
	sim->tx->addItem(QObject::tr("%1: PRI %2, ADDR %3, LEN %4: %5").arg(ret).arg(pri).arg(addr, 0, 16).arg(data.size()).arg(Simulator::dataString(data.data(), data.size())));
}

void RXTask::run()
{
loop:
	llc_packet_t pkt;
	xQueueReceive(llc_rx, &pkt, -1);
	sim->rx->addItem(QObject::tr("PRI %1, ADDR %2, LEN %3: %4").arg(pkt.pri).arg(pkt.addr, 0, 16).arg(pkt.len).arg(Simulator::dataString((char *)pkt.payload, pkt.len)));
	if (pkt.ptr)
		sim->memFree(pkt.ptr);
	goto loop;
}
