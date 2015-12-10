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
	socket = new QUdpSocket(this);

	QVBoxLayout *lay = new QVBoxLayout(this);
	QHBoxLayout *hLay;
	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(this->netAddr = new QLineEdit("0"));
	hLay->addWidget(this->macAddr = new QLineEdit("0"));
	hLay->addWidget(this->destAddr = new QLineEdit("0"));

	lay->addWidget(input = new QLineEdit(tr("Input...")));

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(pri = new QCheckBox(tr("Request &ACK")));
	hLay->addWidget(send = new QPushButton(tr("&Transmit")));
	send->setDefault(true);

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(tx = new QListWidget);
	hLay->addWidget(rx = new QListWidget);
	hLay->addWidget(mem = new QListWidget);

	lay->addLayout(hLay = new QHBoxLayout);
	hLay->addWidget(transmitLog = new QListWidget);
	hLay->addWidget(receivedLog = new QListWidget);

	addr.mac = 0;
	addr.net = 0;
	updateAddr();

	sim_start();

	RXTask *task = new RXTask;
	task->setAutoDelete(true);
	QThreadPool::globalInstance()->start(task);

	connect(send, SIGNAL(clicked(bool)), this, SLOT(write()));
	connect(socket, SIGNAL(readyRead()), this, SLOT(read()));
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
	TXTask *task = new TXTask(pri->isChecked() ? DL_DATA_ACK : DL_UNITDATA, destAddr->text().toUInt(0, 16), str.length(), str.toLocal8Bit().data());
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
		receivedLog->scrollToBottom();
		sim_rx_handle(addr, datagram.data(), len);
	}
}

void Simulator::updateAddr()
{
	addr.mac = macAddr->text().toUInt(0, 16);
	addr.net = netAddr->text().toUInt(0, 16);
	socket->close();
	if (socket->bind(BASE + addr.mac)) {
		QString str = tr("Simulator, mac: %1, net: %2").arg(addr.mac, 0, 16).arg(addr.net, 0, 16);
		setWindowTitle(str);
	}
}

void Simulator::transmit(uint8_t addr, void *ptr, int len)
{
	socket->writeDatagram((char *)ptr, len, QHostAddress::LocalHost, BASE + addr);
	socket->waitForBytesWritten();
	transmitLog->addItem(tr("ADDR %1, LEN %2: %3").arg(addr, 0, 16).arg(len).arg(dataString((char *)ptr, len)));
	emit scrollTransmitLog();
}

void *Simulator::memAlloc(int size)
{
	void *ptr = malloc(size);
	mapMem[ptr] = size;
	emit updateMem();
	return ptr;
}

void Simulator::memFree(void *ptr)
{
	if (mapMem.remove(ptr) != 1) {
		emit error(tr("Error freeing memory at 0x%1").arg((quint64)ptr, 0, 16));
		return;
	}
	free(ptr);
	emit updateMem();
}

void TXTask::run()
{
	uint8_t ret = llc_tx(pri, addr, data.size(), data.data());
	sim->tx->addItem(QObject::tr("(%1) P: %2, A: %3, L: %4 => %5").arg(ret).arg(pri).arg(addr, 0, 16).arg(data.size()).arg(Simulator::dataString(data.data(), data.size())));
	sim->txScroll();
}

void RXTask::run()
{
loop:
	llc_packet_t pkt;
	xQueueReceive(llc_rx, &pkt, -1);
	sim->rx->addItem(QObject::tr("P: %1, A: %2, L: %3 => %4").arg(pkt.pri).arg(pkt.addr, 0, 16).arg(pkt.len).arg(Simulator::dataString((char *)pkt.payload, pkt.len)));
	sim->rxScroll();
	if (pkt.ptr)
		sim->memFree(pkt.ptr);
	goto loop;
}
