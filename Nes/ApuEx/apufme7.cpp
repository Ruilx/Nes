#include "apufme7.h"

ApuFme7::ApuFme7(QObject *parent) : QObject(parent)
{
	// Provisionally set
	this->reset(ApuClock, 22050);
}

ApuFme7::~ApuFme7()
{

}

void ApuFme7::reset(float clock, int rate)
{
	Clear(envelope);
	Clear(noise);

	for(int i = 0; i < 3; i++){
		Clear(op[i]);
	}

	this->envelope.envtbl = envelopeTable[0];
	this->envelope.envstep = envstepTable[0];

	this->noise.noiserange = 1;
	this->noise.noiseout = 0xFF;

	this->address = 0;

	// Volume to voltage
	double out = 0x1FFF;
	for(int i = 31; i > 1; i--){
		this->volTable[i] = int(out + 0.5);
		out /= 1.188502227; // = 10 ^ (1.5/20) = 1.5dB
	}
	this->volTable[1] = 0;
	this->volTable[0] = 0;

	this->setup(clock, rate);
}

void ApuFme7::setup(float clock, int rate)
{
	this->cpuClock = clock;
	this->cycleRate = int((clock / 16.0f) * (1 << 16) / rate);
}

void ApuFme7::write(quint16 addr, quint8 data)
{
	if(addr == 0xC000){
		address = data;
	}else if(addr == 0xE000){
		uint8 chaddr = address;
		switch(chaddr){
			case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05:
				{
					Channel &ch = op[chaddr >> 1];
					ch.reg[chaddr & 0x01] = data;
					ch.freq = Int2Fix((int(ch.reg[1] & 0x0F) << 8) + ch.reg[0] +1);
				}
				break;
			case 0x06:
				noise.freq = Int2Fix(int(data & 0x1F) +1);
				break;
			case 0x07:
				{
					for(int i = 0; i < 3; i++){
						op[i].enable = data & (1 << i);
						op[i].noiseOn = data & (8 << i);
					}
				}
				break;
			case 0x08: case 0x09: case 0x0A:
				{
					Channel &ch = op[chaddr & 0x03];
					ch.reg[2] = data;
					ch.envOn = data & 0x10;
					ch.volume = (data & 0x0F) * 2;
				}
				break;
			case 0x0B: case 0x0C:
				envelope.reg[chaddr - 0x0B] = data;
				envelope.freq = Int2Fix((int(envelope.reg[1] & 0x0F) << 8) + envelope.reg[0] +1);
				break;
			case 0x0D:
				envelope.envtbl = envelopeTable[data & 0x0F];
				envelope.envstep = envstepTable[data & 0x0F];
				envelope.envadr = 0;
				break;
		}
	}
}

int ApuFme7::process(int channel)
{
	if(channel < 3){
		return this->channelRender(op[channel]);
	}else if(channel == 3){
		// Always be referred to as the ch0-2 from calling once ch3
		this->envelopeRender();
		this->noiseRender();
	}
	return 0;
}

int ApuFme7::getFreq(int channel)
{
	if(channel < 3){
		Channel *ch = &op[channel];
		if(ch->enable || !ch->freq){
			return 0;
		}
		if(ch->envOn){
			if(!envelope.volume){
				return 0;
			}
		}else{
			if(!ch->volume){
				return 0;
			}
		}
		return int(256.0f * cpuClock / (float(Fix2Int(ch->freq)) * 16.0f));
	}
	return 0;
}

int ApuFme7::getStateSize()
{
	return sizeof(uint8) + sizeof(envelope) + sizeof(noise) + 3 * sizeof(op);
}

void ApuFme7::saveState(quint8 *p)
{
	SetByte(p, address);
	SetBlock(p, &envelope, sizeof(envelope));
	SetBlock(p, &noice, sizeof(noise));
	SetBlock(p, op, 3 * sizeof(op));
}

void ApuFme7::loadState(quint8 *p)
{
	GetByte(p, address);
	GetBlock(p, &envelope, sizeof(envelope));
	GetBlock(p, &noice, sizeof(noise));
	GetBlock(p, op, 3 * sizeof(op));
}

void ApuFme7::envelopeRender()
{
	if(!envelope.freq){
		return;
	}
	envelope.phaseacc -= cycleRate;
	if(envelope.phaseacc >= 0){
		return;
	}
	while(envelope.phaseacc < 0){
		envelope.phaseacc += envelope.freq;
		envelope.envadr += envelope.envstep[envelope.envadr];
	}
	envelope.volume = envelope.envtbl[envelope.envadr];
}

void ApuFme7::noiseRender()
{
	if(!noise.freq){
		return;
	}
	noise.phaseacc -= cycleRate;
	if(noise.phaseacc >= 0){
		return;
	}
	while(noise.phaseacc < 0){
		noise.phaseacc += noise.freq;
		if((noise.noiserange +1) & 0x02){
			noise.noiseout = ~noise.noiseout;
		}
		if(noise.noiserange & 0x01){
			noise.noiserange ^= 0x028000;
		}
		noise.noiserange >>= 1;
	}
}

int ApuFme7::channelRender(ApuFme7::Channel &ch)
{
	int output = 0;
	int volume = 0;
	if(ch.enable){
		return 0;
	}
	if(!ch.freq){
		return 0;
	}
	ch.phaseacc -= cycleRate;
	while(ch.phaseacc < 0){
		ch.phaseacc += ch.freq;
		ch.adder++;
	}
	volume = ch.envOn? volTable[envelope.volume]: volTable[ch.volume +1];
	if(ch.adder & 0x01){
		output += volume;
	}else{
		output -= volume;
	}
	if(!ch.noiseOn){
		if(noise.noiseout){
			output += volume;
		}else{
			output -= volume;
		}
	}
	ch.outputVol = output;
	return ch.outputVol;
}



















