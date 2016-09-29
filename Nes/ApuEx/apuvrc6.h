#ifndef APUVRC6_H
#define APUVRC6_H

#include <QObject>

#include "typedef.h"
#include "macro.h"

#include "apuinterface.h"

class ApuVrc6 : public QObject,  public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuVrc6(QObject *parent = 0);
	~ApuVrc6();

	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(quint16 addr, quint8 data);
	int process(int channel);
	int getFreq(int channel);
	int getStateSize();
	void saveState(uint8 *p);
	void loadState(uint8 *p);
protected:
	typedef struct{
		uint8 reg[3];
		uint8 enable;
		uint8 gate;
		uint8 volume;
		int phaseacc;
		int freq;
		int outputVol;
		uint8 adder;
		uint8 dutyPos;
	} Rectangle, * lpRectangle;
	typedef struct{
		uint8 reg[3];
		uint8 enable;
		uint8 volume;
		int phaseacc;
		int freq;
		int outputVol;
		uint8 adder;
		uint8 accum;
		uint8 phaseaccum;
	}Sawtooth, *lpSawtooth;
	int rectangleRender(Rectangle &ch);
	int sawtoothRender(Sawtooth &ch);

	Rectangle ch0;
	Rectangle ch1;
	Sawtooth ch2;

	int cycleRate;
	float cpuClock;


signals:

public slots:
};


#endif // APUVRC6_H
