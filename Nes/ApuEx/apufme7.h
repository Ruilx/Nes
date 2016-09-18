#ifndef APUFME7_H
#define APUFME7_H

#include <QObject>
#include "macro.h"
#include "typedef.h"
#include "apuinterface.h"

class ApuFme7 : public QObject, public ApuInterface
{
	Q_OBJECT
public:
	explicit ApuFme7(QObject *parent = 0);
	~ApuFme7();

	void reset(float clock, int rate);
	void setup(float clock, int rate);
	void write(quint16 addr, quint8 data);
	int process(int channel);
	int getFreq(int channel);
	int getStateSize();
	void saveState(quint8 *p);
	void loadState(quint8 *p);
protected:
	typedef struct{
		uint8 reg[3];
		uint8 volume;
		int freq;
		int phaseacc;
		int envadr;
		uint8 *envtbl;
		int8 *envstep;
	}Envelope, * lpEnvelope;
	typedef struct{
		int freq;
		int phaseacc;
		int noiserange;
		uint8 noiseout;
	}Noise, *lpNoise;
	typedef struct{
		uint8 reg[3];
		uint8 enable;
		uint8 envOn;
		uint8 noiseOn;
		uint8 adder;
		uint8 volume;
		int freq;
		int phaseacc;
		int outputVol;
	}Channel, *lpChannel;

	void envelopeRender();
	void noiseRender();

	int channelRender(Channel &ch);

	Envelope envelope;
	Noise noise;
	Channel op[3];

	uint8 address;
	 int volTable[0x20];
	 int cycleRate;

	 float cpuClock;

	 // Tables
	 static uint8 envelopePulse0[] = {
		 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
		 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	 };
	 static uint8 envelopePulse1[] = {
			   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		 0x00
	 };
	 static uint8 envelopePulse2[] = {
		 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
		 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x1F
	 };
	 static uint8 envelopePulse3[] = {
			   0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		 0x1F
	 };
	 static int8 envstepPulse[] = {
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 0
	 };

	 static uint8 envelopeSawtooth0[] = {
		 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
		 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	 };
	 static uint8 envelopeSawtooth1[] = {
		 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
	 };
	 static int8 envstepSawtooth[] = {
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, -15
	 };

	 static uint8 envelopeTriangle0[] ={
		 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
		 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
		 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
	 };
	 static uint8 envelopeTriangle1[] = {
		 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
		 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
		 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
		 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
		 0x1F, 0x1E, 0x1D, 0x1C, 0x1B, 0x1A, 0x19, 0x18,
		 0x17, 0x16, 0x15, 0x14, 0x13, 0x12, 0x11, 0x10,
		 0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08,
		 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00
	 };
	 static int8 envstepTriangle[] = {
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, 1,
		 1, 1, 1, 1, 1, 1, 1, -31
	 };

	 static uint8 *envelopeTable[16] = {
		 envelopePulse0,    envelopePulse0, envelopePulse0,    envelopePulse0,
		 envelopePulse1,    envelopePulse1, envelopePulse1,    envelopePulse1,
		 envelopeSawtooth0, envelopePulse0, envelopeTriangle0, envelopePulse2,
		 envelopeSawtooth1, envelopePulse3, envelopeTriangle1, envelopePulse1
	 };
	 static int8 *envstepTable[16] = {
		 envstepPulse,    envstepPulse, envstepPulse,    envstepPulse,
		 envstepPulse,    envstepPulse, envstepPulse,    envstepPulse,
		 envstepSawtooth, envstepPulse, envstepTriangle, envstepPulse,
		 envstepSawtooth, envstepPulse, envstepTriangle, envstepPulse
	 };
signals:

public slots:
};

#define ChannelVolShift 8

#endif // APUFME7_H
