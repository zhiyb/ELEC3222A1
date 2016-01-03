#include "simulator.h"
#include "../net_layer.h"
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

QString Simulator::dataString(const char *ptr, uint8_t len)
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
	mutexMem.lock();
	QMap<void *, int>::const_iterator it = mapMem.constBegin();
	while (it != mapMem.constEnd()) {
		mem->addItem(tr("0x%1: SIZE %2").arg((quint64)it.key(), 0, 16).arg(it.value()));
		it++;
	}
	mutexMem.unlock();
	mem->scrollToBottom();
}

void Simulator::txScroll()
{
	emit scrollTXLog();
}

void Simulator::rxScroll()
{
	emit scrollRXLog();
}

void Simulator::errorMessage(QString str)
{
	QMessageBox::critical(this, windowTitle(), str);
}

Simulator::Simulator(QWidget *parent) : QWidget(parent)
{
	sim = this;

	QVBoxLayout *lay = new QVBoxLayout(this);
	QHBoxLayout *hLay;
	QVBoxLayout *vLay;

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(new QLabel(tr("NET address")));
	hLay->addWidget(this->netAddr = new QLineEdit("0"));
	hLay->addWidget(new QLabel(tr("MAC address")));
	hLay->addWidget(this->macAddr = new QLineEdit("0"));
	hLay->addWidget(new QLabel(tr("DEST address")));
	hLay->addWidget(this->destAddr = new QLineEdit("0"));

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(new QLabel(tr("Data")));
	hLay->addWidget(input = new QLineEdit(tr("Input...")));

	lay->addLayout(hLay = new QHBoxLayout);
#ifdef PRI_ENABLE
	hLay->addWidget(pri = new QCheckBox(tr("Request &ACK")));
#endif
	hLay->addWidget(send = new QPushButton(tr("&Transmit")));
	send->setDefault(true);

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(tx = new QListWidget);
	hLay->addWidget(rx = new QListWidget);

	hLay->addLayout(vLay = new QVBoxLayout);
	vLay->addWidget(memClear = new QCheckBox(tr("&Clear on free")));
	memClear->setChecked(true);
	vLay->addWidget(mem = new QListWidget);

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(transmitLog = new QListWidget);
	hLay->addWidget(receivedLog = new QListWidget);

	addr.mac = 0;
	addr.net = 0;
#if 1
	gSocket.setSocketOption(QAbstractSocket::MulticastLoopbackOption, 1);
	if (!gSocket.bind(BASE + MAC_BROADCAST, QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint))
		qFatal("Error binding socket");
#endif
	updateAddr();

	sim_start();

	RXTask *task = new RXTask;
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);

	connect(send, SIGNAL(clicked(bool)), this, SLOT(write()));
	connect(&socket, SIGNAL(readyRead()), this, SLOT(read()));
	connect(&gSocket, SIGNAL(readyRead()), this, SLOT(read()));
	connect(this, SIGNAL(scrollTransmitLog()), transmitLog, SLOT(scrollToBottom()));
	connect(this, SIGNAL(updateMem()), this, SLOT(updateMemList()));
	connect(this, SIGNAL(scrollTXLog()), tx, SLOT(scrollToBottom()));
	connect(this, SIGNAL(scrollRXLog()), rx, SLOT(scrollToBottom()));
	connect(macAddr, SIGNAL(textChanged(QString)), this, SLOT(updateAddr()));
	connect(netAddr, SIGNAL(textChanged(QString)), this, SLOT(updateAddr()));
}

Simulator::~Simulator()
{
	sim_end();
}

void Simulator::write()
{
	QString str(input->text());
#ifdef PRI_ENABLE
	TXTask *task = new TXTask(pri->isChecked() ? DL_DATA_ACK : DL_UNITDATA, destAddr->text().toUInt(0, 16), str.length(), str.toLocal8Bit().data());
#else
	TXTask *task = new TXTask(0, destAddr->text().toUInt(0, 16), str.length(), str.toLocal8Bit().data());
#endif
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);
}

void Simulator::read()
{
	while (socket.hasPendingDatagrams()) {
		QByteArray data;
		data.resize(socket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		socket.readDatagram(data.data(), data.size(),
					&sender, &senderPort);

		uint8_t pri = data.at(0);
		data.remove(0, 1);
		uint8_t len = data.size();
		uint8_t addr = senderPort - BASE;
		receivedLog->addItem(tr("P: %1, A: %2, L: %3 => %4").arg(pri).arg(addr, 0, 16).arg(len).arg(dataString(data.data(), len)));
		receivedLog->scrollToBottom();
		sim_rx_handle(pri, addr, data);
	}
	while (gSocket.hasPendingDatagrams()) {
		QByteArray data;
		data.resize(gSocket.pendingDatagramSize());
		QHostAddress sender;
		quint16 senderPort;

		gSocket.readDatagram(data.data(), data.size(),
					&sender, &senderPort);

		uint8_t pri = DL_UNITDATA;
		data.remove(0, 1);
		uint8_t len = data.size();
		uint8_t addr = senderPort - BASE;
		receivedLog->addItem(tr("P: %1, A: %2, L: %3 => %4").arg(pri).arg(addr, 0, 16).arg(len).arg(dataString(data.data(), len)));
		receivedLog->scrollToBottom();
		sim_rx_handle(pri, addr, data);
	}
}

void Simulator::updateAddr()
{
	addr.mac = macAddr->text().toUInt(0, 16);
	addr.net = netAddr->text().toUInt(0, 16);
	socket.close();
	if (socket.bind(BASE + addr.mac)) {
		QString str = tr("Simulator, mac: %1, net: %2").arg(addr.mac, 0, 16).arg(addr.net, 0, 16);
		setWindowTitle(str);
	}
}

void Simulator::transmit(uint8_t pri, uint8_t addr, QByteArray &data)
{
	transmitLog->addItem(tr("P: %1, A: %2, L: %3 => %4").arg(pri).arg(addr, 0, 16).arg(data.size()).arg(dataString(data.data(), data.size())));
	data.prepend(pri);
	socket.writeDatagram(data, QHostAddress::LocalHost, BASE + addr);
	socket.waitForBytesWritten();
	emit scrollTransmitLog();
}

void *Simulator::memAlloc(int size)
{
	void *ptr = malloc(size);
	mutexMem.lock();
	mapMem[ptr] = size;
	mutexMem.unlock();
	emit updateMem();
	return ptr;
}

void Simulator::memFree(void *ptr)
{
	if (memClear->isChecked()) {
		QMutexLocker locker(&mutexMem);
		if (mapMem.remove(ptr) != 1) {
			emit error(tr("Error freeing memory at 0x%1").arg((quint64)ptr, 0, 16));
			return;
		}
	}
	free(ptr);
	emit updateMem();
}

void TXTask::run()
{
	uint8_t ret = net_tx(addr, data.size(), data.data());
	sim->tx->addItem(QObject::tr("(%1) A: %2, L: %3 => %4").arg(ret).arg(addr, 0, 16).arg(data.size()).arg(Simulator::dataString(data.data(), data.size())));
	sim->txScroll();
}

void RXTask::run()
{
loop:
	net_packet_t pkt;
	xQueueReceive(net_rx, &pkt, -1);
	sim->rx->addItem(QObject::tr("A: %1, L: %2 => %3").arg(pkt.addr, 0, 16).arg(pkt.len).arg(Simulator::dataString((char *)pkt.payload, pkt.len)));
	sim->rxScroll();
	if (pkt.ptr)
		sim->memFree(pkt.ptr);
	goto loop;
}
