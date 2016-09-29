#include "apummc5.h"

#define RectangleVolShift 8
#define DaoutVolShift 6

ApuMmc5::ApuMmc5(QObject *parent) : QObject(parent)
{
	// Provisionally set
	this->reset(ApuClock, 22050);
}

ApuMmc5::~ApuMmc5()
{

}

void ApuMmc5::reset(float clock, int rate)
{
	Clear(this->ch0);
	Clear(this->ch1);

	this->reg5010 = 0;
	this->reg5011 = 0;
	this->reg5015 = 0;
	this->syncReg5015 = 0;
	this->frameCycle = 0;

	this->setup(clock, rate);

	for(uint16 addr = 0x5000; addr <= 0x5015; addr++){
		this->write(addr, 0);
	}
}

void ApuMmc5::setup(float clock, int rate)
{
	this->cpuClock = clock;
	this->cycleRate = int(clock * 65536.0f / float(rate));
	// Create Tables
	int i;
	int samples = int(clock * 65536.0f / float(rate));

	for(i = 0; i < 16; i++){
		this->decayLut[i] = (i + 1) * samples * 5;
	}
	for(i = 0; i < 32; i++){
		this->vblLut[i] = this->vblLength[i] * samples * 5;
	}
}

void ApuMmc5::write(quint16 addr, quint8 data)
{
	switch(addr){
		// MMC5 Ch0 rectangle
		case 0x5000:
			ch0.reg[0] = data;
			ch0.volume = data & 0x0F;
			ch0.holdnote = data & 0x20;
			ch0.fixedEnvelope = data & 0x10;
			ch0.envDecay = decayLut[data & 0x0F];
			ch0.dutyFlip = dutyLut[data >> 6];
			break;
		case 0x5001:
			ch0.reg[1] = data;
			break;
		case 0x5002:
			ch0.reg[2] = data;
			ch0.freq = Int2Fix(((ch0.reg[3] & 0x07) << 8) + data +1);
			break;
		case 0x5003:
			ch0.reg[3] = data;
			ch0.vblLength = this->vblLut[data >> 3];
			ch0.envVol = 0;
			ch0.freq = Int2Fix(((data & 0x07) << 8) + ch0.reg[2] +1);
			if(reg5015 & 0x01){
				ch0.enable = 0xFF;
			}
			break;
			// MMC5 ch1 rectangle
		case 0x5004:
			ch1.reg[0] = data;
			ch1.volume = data & 0x0F;
			ch1.holdnote = data & 0x20;
			ch1.fixedEnvelope = data & 0x10;
			ch1.envDecay = this->decayLut[data & 0x0F];
			ch1.dutyFlip = this->dutyLut[data >> 6];
			break;
		case 0x5005:
			ch1.reg[1] = data;
			break;
		case 0x5006:
			ch1.reg[2] = data;
			ch1.freq = Int2Fix(((ch1.reg[3] & 0x07) << 8) + data +1);
			break;
		case 0x5007:
			ch1.reg[3] = data;
			ch1.vblLength = this->vblLut[data >> 3];
			ch1.envVol = 0;
			ch1.freq = Int2Fix(((data & 0x07) << 8) + ch1.reg[2] +1);
			if(this->reg5015 & 0x02){
				ch1.enable = 0xFF;
			}
			break;
		case 0x5010:
			this->reg5010 = data;
			break;
		case 0x5011:
			this->reg5011 = data;
			break;
		case 0x5012:
		case 0x5013:
		case 0x5014:
			break;
		case 0x5015:
			this->reg5015 = data;
			if(this->reg5015 & 0x01){
				ch0.enable = 0xFF;
			}else{
				ch0.enable = 0;
				ch0.vblLength = 0
			}
			if(this->reg5015 & 0x02){
				ch1.enable = 0xFF;
			}else{
				ch1.enable = 0;
				ch1.vblLength = 0;
			}
			break;
	}
}

int ApuMmc5::process(int channel)
{
	switch(channel){
		case 0:
			return rectangleRender(ch0);
			break;
		case 1:
			return rectangleRender(ch1);
			break;
		case 2:
			return int(this->reg5011) << DaoutVolShift;
			break;
	}
	return 0;
}

void ApuMmc5::syncWrite(quint16 addr, quint8 data)
{
	switch(addr){
		// MMC5 Ch0 rectangle
		case 0x5000:
			sch0.reg[0] = data;
			sch0.holdnote = data & 0x20;
			break;
		case 0x5001:
		case 0x5002:
			sch0.reg[addr & 0x03] = data;
			break;
		case 0x5003:
			sch0.reg[3] = data;
			sch0.vblLength = this->vblLut[data >> 3];
			if(this->syncReg5015 & 0x01){
				sch0.enable = 0xFF;
			}
			break;
			// MMC5 ch1 rectangle
		case 0x5004:
			sch1.reg[0] = data;
			sch1.holdnote = data & 0x20;
			break;
		case 0x5005:
		case 0x5006:
			sch1.reg[addr & 0x03] = data;
			break;
		case 0x5007:
			sch1.reg[3] = data;
			sch1.vblLength = this->vblLut[data >> 3];
			if(this->syncReg5015 & 0x02){
				sch1.enable = 0xFF;
			}
			break;
		case 0x5010:
		case 0x5011:
		case 0x5012:
		case 0x5013:
		case 0x5014:
			break;
		case 0x5015:
			this->syncReg5015 = data;
			if(this->syncReg5015 & 0x01){
				sch0.enable = 0xFF;
			}else{
				sch0.enable = 0;
				sch0.vblLength = 0
			}
			if(this->syncReg5015 & 0x02){
				sch1.enable = 0xFF;
			}else{
				sch1.enable = 0;
				sch1.vblLength = 0;
			}
			break;
	}
}

quint8 ApuMmc5::syncRead(quint16 addr)
{
	uint8 data = 0;
	if(addr == 0x5015){
		if(sch0.enable && sch0.vblLength > 0){
			data |= (1 << 0);
		}
		if(sch1.enable && sch1.vblLength > 0){
			data |= (1 << 1);
		}
	}
	return data;
}

bool ApuMmc5::sync(int cycles)
{
	this->frameCycle += cycles;
	if(this->frameCycle >= 7457 * 5 / 2){
		this->frameCycle -= 7457 * 5 / 2;

		if(sch0.enable && !sch0.holdnote){
			if(sch0.vblLength){
				sch0.vblLength--;
			}
		}
		if(sch1.enable && !sch1.holdnote){
			if(sch1.vblLength){
				sch1.vblLength--;
			}
		}
	}
	return false;
}

int ApuMmc5::getFreq(int channel)
{
	if(channel == 0 || channel == 1){
		Rectangle *ch;
		if(channel == 0){
			ch = &ch0;
		}else{
			ch = &ch1;
		}

		if(!ch->enable || ch->vblLength <= 0){
			return 0;
		}
		if(ch->freq < Int2Fix(8)){
			return 0;
		}
		if(ch->fixedEnvelope){
			if(!ch->volume){
				return 0;
			}
		}else{
			if(!(0x0F - ch->envVol)){
				return 0;
			}
		}
		return int(256.0f * this->cpuClock / (float(Fix2Int(ch->freq)) * 16.0f));
	}
	return 0;
}

int ApuMmc5::getStateSize()
{
	return 3 * sizeof(uint8) + sizeof(ch0) + sizeof(ch1) + sizeof(sch0) + sizeof(sch1);
}

void ApuMmc5::saveState(quint8 *p)
{
	SetByte(p, this->reg5010);
	SetByte(p, this->reg5011);
	SetByte(p, this->reg5015);

	SetBlock(p, &ch0, sizeof(ch0));
	SetBlock(p, &ch1, sizeof(ch1));

	SetByte(p, this->syncReg5015);
	SetBlock(p, &sch0, sizeof(sch0));
	SetBlock(p, &sch1, sizeof(sch1));
}

void ApuMmc5::loadState(quint8 *p)
{
	GetByte(p, this->reg5010);
	GetByte(p, this->reg5011);
	GetByte(p, this->reg5015);

	GetBlock(p, &ch0, sizeof(ch0));
	GetBlock(p, &ch1, sizeof(ch1));

	GetByte(p, this->syncReg5015);
	GetBlock(p, &sch0, sizeof(sch0));
	GetBlock(p, &sch1, sizeof(sch1));
}

int ApuMmc5::rectangleRender(ApuMmc5::Rectangle &ch)
{
	if(!ch.enable || ch.vblLength <= 0){
		return 0;
	}
	// vbl Length counter
	if(!ch.holdnote){
		ch.vblLength -= 5;
	}
	// envelope unit
	ch.envPhase -= 5 * 4;
	while(ch.envPhase < 0){
		ch.envPhase += ch.envDecay;
		if(ch.holdnote){
			ch.envVol = (ch.envVol +1) & 0x0F;
		}else if(ch.envVol < 0x0F){
			ch.envVol++;
		}
	}
	if(ch.freq < Int2Fix(8)){
		return 0;
	}
	int volume;
	if(ch.fixedEnvelope){
		volume = int(ch.volume);
	}else{
		volume = int(0x0F - ch.envVol);
	}
	int output = volume << RectangleVolShift;
	ch.phaseacc -= cycleRate;
	if(ch.phaseacc >= 0){
		if(ch.adder < ch.dutyFlip){
			ch.outputVol = output;
		}else{
			ch.outputVol = -output;
		}
		return ch.outputVol;
	}
	if(ch.freq > cycleRate){
		ch.phaseacc += ch.freq;
		ch.adder = (ch.adder +1) & 0x0F;
		if(ch.adder < ch.dutyFlip){
			ch.outputVol = output;
		}else{
			ch.outputVol = -output;
		}
	}else{
		// weighted average
		int numTimes = 0;
		int total = 0;
		while(ch.phaseacc < 0){
			ch.phaseacc += ch.freq;
			ch.adder = (ch.adder +1) & 0x0F;
			if(ch.adder < ch.dutyFlip){
				total += output;
			}else{
				total -= output;
			}
		}
		ch.outputVol = total / numTimes;
	}
	return ch.outputVol;
}
