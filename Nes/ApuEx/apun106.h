#ifndef APUN106_H
#define APUN106_H

#include <QObject>
#include "apuinterface.h"
#include "macro.h"
#include "typedef.h"

class ApuN106 : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuN106(QObject *parent = 0);
	~ApuN106();
	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(quint16 addr, quint8 data);
	uint8 read(quint16 addr);
	int process(int channel);
	int getFreq(int channel);
	int getStateSize();
	void saveState(uint8 *p);
	void loadState(uint8 *p);
protected:
	typedef struct{
		int phaseacc;
		uint32 freq;
		uint32 phase;
		uint32 tonelen;
		int output;
		uint8 toneadr;
		uint8 volupdate;
		uint8 vol;
		uint8 databuf;
	} Channel;

	Channel op[8];
	float cpuClock;
	uint32 cycleRate;
	uint8 addrinc;
	uint8 address;
	uint8 channelUse;
	uint8 tone[0x100];
	int channelRender(Channel &ch);

signals:

public slots:
};

#define ChannelVolShift 6

#endif // APUN106_H
