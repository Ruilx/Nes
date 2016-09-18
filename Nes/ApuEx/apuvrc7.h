#ifndef APUVRC7_H
#define APUVRC7_H

#include <QObject>

#include "typedef.h"
#include "macro.h"

#include "apuinterface.h"
#include "Emu2413/emu2413.h"

class ApuVrc7 : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuVrc7(QObject *parent = 0);
	~ApuVrc7();

	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(uint16 addr, uint8 data);
	int process(int channel);
	int getFreq(int channel);
protected:
	OPLL* Vrc7Opll;
	uint8 address;

signals:

public slots:
};

#endif // APUVRC7_H
