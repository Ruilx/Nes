#include "apuvrc6.h"

ApuVrc6::ApuVrc6(QObject *parent) : QObject(parent)
{
	// Provisionally set
	this->reset(ApuClock, 22050);
}

void ApuVrc6::reset(float clock, int rate)
{
	Clear(ch0);
	Clear(ch1);
	Clear(ch2);

	this->setup(clock, rate);
}

void ApuVrc6::setup(float clock, int rate)
{
	this->cpuClock = clock;
	this->cycleRate = int(clock * 65536.0f / float(rate));
}

void ApuVrc6::write(quint16 addr, quint8 data)
{
	switch(addr){
		// VCR6 Ch0 retangle
		case 0x9000:
			ch0.reg[0] = data;
			ch0.gate = data & 0x80;
			ch0.volume = data & 0x0F;
			ch0.dutyPos = (data >> 4) & 0x07;
			break;
		case 0x9001:
			ch0.reg[1] = data;
			ch0.freq = Int2Fix((((ch0.reg[2] & 0x0F) << 8) | data) +1);
			break;
		case 0x9002:
			ch0.reg[2] = data;
			ch0.enable = data & 0x80;
			ch0.freq = Int2Fix((((data & 0x0F) << 8) | ch0.reg[1]) +1);
			break;
			// VRC6 Ch1 rectangle
		case 0xA000:
			ch1.reg[0] = data;
			ch1.gate = data & 0x80;
			ch1.volume = data & 0x0F;
			ch1.dutyPos = (data >> 4) & 0x07;
			break;
		case 0xA002:
			ch1.reg[2] = data;
			ch1.enable = data & 0x80;
			ch1.freq = Int2Fix((((data & 0x0F) << 8) | ch1.reg[1]) + 1);
			break;
			// VRC6 Ch2 sawtooth
		case 0xB000:
			ch2.reg[1] = data;
			ch2.phaseaccum = data & 0x3F;
			break;
		case 0xB001:
			ch2.reg[1] = data;
			ch2.freq = Int2Fix((((ch2.reg[2] & 0x0F) << 8) | data) +1);
			break;
		case 0xB002:
			ch2.reg[2] = data;
			ch2.enable = data & 0x80;
			ch2.freq = Int2Fix((((data & 0x0F) << 8) | ch2.reg[1]) + 1);
			//ch2.adder = 0; //Cause of the noise is cleared
			//ch2.accum = 0; //Cause of the noise is cleared
			break;
	}
}

int ApuVrc6::process(int channel)
{
	switch(channel){
		case 0:
			return rectangleRender(ch0);
			break;
		case 1:
			return rectangleRender(ch1);
			break;
		case 2:
			return sawtoothRender(ch2);
			break;
	}
	return 0;
}

int ApuVrc6::getFreq(int channel)
{
	if(channel == 0 || channel == 1){
		Rectangle *ch;
		if(channel == 0){
			ch = &ch0;
		}else{
			ch = &ch1;
		}
		if(!ch->enable || ch->gate || !ch->volume){
			return 0;
		}
		if(ch->freq < Int2Fix(8)){
			return 0;
		}
		return int(256.0f * this->cpuClock / (float(Fix2Int(ch->freq)) * 16.0f));
	}
	if(channel == 2){
		Sawtooth *ch = &ch2;
		if(!ch->enable || !ch->phaseaccum){
			return 0;
		}
		if(ch->freq < Int2Fix(8)){
			return 0;
		}
		return int(256.0f * this->cpuClock / (float(Fix2Int(ch->freq)) * 14.0f));
	}
	return 0;
}

int ApuVrc6::getStateSize()
{
	return sizeof(ch0) + sizeof(ch1) + sizeof(ch2);
}

void ApuVrc6::saveState(quint8 *p)
{
	SetBlock(p, &ch0, sizeof(ch0));
	SetBlock(p, &ch1, sizeof(ch1));
	SetBlock(p, &ch2, sizeof(ch2));
}

void ApuVrc6::loadState(quint8 *p)
{
	GetBlock(p, &ch0, sizeof(ch0));
	GetBlock(p, &ch1, sizeof(ch1));
	GetBlock(p, &ch2, sizeof(ch2));
}

int ApuVrc6::rectangleRender(ApuVrc6::Rectangle &ch)
{
	// Enable?
	if(!ch.enable){
		ch.outputVol = 0;
		ch.adder = 0;
		return ch.outputVol;
	}
	// Digitized output
	if(ch.gate){
		ch.outputVol = ch.volume << RectangleVolShift;
		return ch.outputVol;
	}
	// Above a certain frequency will not be treated (dead)
	if(ch.freq < Int2Fix(8)){
		ch.outputVol = 0;
		return ch.outputVol;
	}
	ch.phaseacc -= this->cycleRate;
	if(ch.phaseacc >= 0){
		return ch.outputVol;
	}
	int output = ch.volume << RectangleVolShift;
	if(ch.freq > this->cycleRate){
		// add 1 step
		ch.phaseacc += ch.freq;
		ch.adder = (ch.adder +1) & 0x0F;
		if(ch.adder <= ch.dutyPos){
			ch.outputVol = output;
		}else{
			ch.outputVol = -output;
		}
	}else{
		// average calculate
		int numTimes = 0;
		int total = 0;
		while(ch.phaseacc < 0){
			ch.phaseacc += ch.freq;
			ch.adder = (ch.adder +1) & 0x0F;
			if(ch.adder <= ch.dutyPos){
				total += output;
			}else{
				total += -output;
			}
			numTimes++;
		}
		ch.outputVol = total / numTimes;
	}
	return ch.outputVol;
}

int ApuVrc6::sawtoothRender(ApuVrc6::Sawtooth &ch)
{
	// Digitized output
	if(!ch.enable){
		ch.outputVol = 0;
		return ch.outputVol;
	}
	// Above a certain frequency will not be treated (dead)
	if(ch.freq < Int2Fix(9)){
		return ch.outputVol;
	}
	ch.phaseacc -= this->cycleRate / 2;
	if(ch.phaseacc >= 0){
		return ch.outputVol;
	}
	if(ch.freq > cycleRate / 2){
		//add 1 step
		ch.phaseacc += ch.freq;
		if(++ch.adder >= 7){
			ch.adder = 0;
			ch.accum = 0;
		}
		ch.accum += ch.phaseaccum;
		ch.outputVol = ch.accum << SawtoothVolShift;
	}else{
		//average calculate
		int numTimes = 0;
		int total = 0;
		while(ch.phaseacc < 0){
			ch.phaseacc += ch.freq;
			if(++ch.adder >= 7){
				ch.adder = 0;
				ch.accum = 0;
			}
			ch.accum += ch.phaseaccum;
			total += ch.accum << SawtoothVolShift;
			numTimes++;
		}
		ch.outputVol = (total / numTimes);
	}
	return ch.outputVol;
}































