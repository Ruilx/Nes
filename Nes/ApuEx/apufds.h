#ifndef APUFDS_H
#define APUFDS_H

#include <QObject>
#include "typedef.h"
#include "macro.h"

#include "apuinterface.h"

class ApuFds : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuFds(QObject *parent = 0);
	~ApuFds();

	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(uint16 addr, uint8 data);
	void read(uint16 addr);
	int process(int channel);

	void syncWrite(uint16 addr, uint8 data);
	uint8 syncRead(uint16 addr);
	bool sync(int cycles);

	int getFreq(int channel);

	int getStateSize();
	void saveState(uint8 *p);
	void loadState(uint8 *p);
protected:
	typedef struct FdsSound_t{
		uint8 reg[0x80];

		uint8 volenvMode; // volume envelope
		uint8 volenvGain;
		uint8 volenvDecay;
		double volenvPhaseacc;

		uint8 swpenvMode; // sweep envelope
		uint8 swpenvGain;
		uint8 swpenvDecay;
		double swpenvPhaseacc;

		// For envelope uint
		uint8 envelopeEnable; // $4083 bit6
		uint8 envelopeSpeed; // $408A

		// For $4089
		uint8 waveSetup; // bit7
		int masterVolume; // bit 1 - 0

		// For Main unit
		int mainWavetable[64];
		uint8 mainEnable;
		int mainFrequency;
		int mainAddr;

		// For Effector(LFO) unit
		uint8 lfoWavetable[64];
		uint8 lfoEnable; // 0:Enable 1:Wavetable setup
		int lfoFrequency;
		int lfoAddr;
		double lfoPhaseacc;

		// For sweep unit
		int sweepBias;

		// Misc
		int nowVolume;
		int nowFreq;
		int output;
	}FdsSound, *lpFdsSound;

	FdsSound fds;
	FdsSound fdsSync;

	int samplingRate;
	int outputBuf[8] = {0};

	// Write Sub
	void writeSub(uint16 addr, uint8 data, FdsSound &ch, double rate);

signals:

public slots:
};

#endif // APUFDS_H
