#include "apun106.h"

ApuN106::ApuN106(QObject *parent) : QObject(parent)
{
	// It initializes the first only TONE
	ZeroMemory(tone, sizeof(tone));

	// Provisionally set
	this->cpuClock = ApuClock;
	this->cycleRate = (uint32)(this->cpuClock * 12.0f * (1 << 20) / (45.0f * 22050.0f));
}

ApuN106::~ApuN106()
{

}

void ApuN106::reset(float clock, int rate)
{
	for(int i=0; i<8; i++){
		Clear(op[i]);
		op[i].tonelen = 0x10 << 18;
	}

	this->address = 0;
	this->addrinc = 1;
	this->channelUse = 8;

	this->setup(clock, rate);

	// TONE is not initialization...
}

void ApuN106::setup(float clock, int rate)
{
	this->cpuClock = clock;
	this->cycleRate = (uint32)(this->cpuClock * 12.0f * (1 << 20) / (45.0f * rate));
}

void ApuN106::write(quint16 addr, quint8 data)
{
	if(addr == 0x4800){
		this->tone[address * 2] = data & 0x0F;
		this->tone[address * 2 + 1] = data >> 4;

		if(address >= 0x40){
			int no = (address - 0x40) >> 3;
			uint32 tonelen;
			Channel &ch = op[no];
			switch(address & 7){
				case 0x00:
					ch.freq = (ch.freq & ~0x000000FF) | (uint32)data;
					break;
				case 0x02:
					ch.freq = (ch.freq & ~0x0000FF00) | ((uint32)(data) << 8);
					break;
				case 0x04:
					ch.freq = (ch.freq & ~0x00030000) | (((uint32)(data) & 0x03) << 16);
					tonelen = (0x20 - (data & 0x1C)) << 18;
					ch.databuf = (data & 0x1C) >> 2;
					if(ch.tonelen != tonelen){
						ch.tonelen = tonelen;
						ch.phase = 0;
					}
					break;
				case 0x06:
					ch.toneadr = data;
					break;
				case 0x07:
					ch.vol = data & 0x0F;
					ch.volupdate = 0xFF;
					if(no == 7){
						channelUse = ((data >> 4) & 0x07) + 1;
					}
					break;
			}
		}
		if(addrinc){
			address = (address +1) & 0x7F;
		}
	}else if(addr == 0xF800){
		address = data & 0x7F;
		addrinc = data & 0x80;
	}
}

quint8 ApuN106::read(quint16 addr)
{
	// $4800 dummy read!!
	if(addr = 0x0000){
		if(addrinc){
			address = (address + 1) & 0x7F;
		}
	}
	return (uint8)(addr >> 8);
}

int ApuN106::process(int channel)
{
	if(channel >= (8 - channelUse) && channel < 8){
		return this->channelRender(op[channel]);
	}
	return 0;
}

int ApuN106::getFreq(int channel)
{
	if(channel < 8){
		channel &= 7;
		if(channel < (8-channelUse)){
			return 0;
		}
		Channel *ch = &op[channel & 0x07];
		if(!ch->freq || !ch->vol){
			return 0;
		}
		int temp = channelUse * (8 - ch->databuf) * 4 * 45;
		if(!temp){
			return 0;
		}
		return int(256.0 * double(this->cpuClock) * 12.0 * ch->freq / (double(0x40000) * temp));
	}
	return 0;
}

int ApuN106::getStateSize()
{
	return 3 * sizeof(uint8) + 8 * sizeof(Channel) + sizeof(tone);
}

void ApuN106::saveState(quint8 *p)
{
	SetByte(p, addrinc);
	SetByte(p, address);
	SetByte(p, channelUse);

	SetBlock(p, op, sizeof(op));
	SetBlock(p, tone, sizeof(tone));
}

void ApuN106::loadState(quint8 *p)
{
	GetByte(p, addrinc);
	GetByte(p, address);
	GetByte(p, channelUse);

	GetBlock(p, op, sizeof(op));
	GetBlock(p, tone, sizeof(tone));
}

int ApuN106::channelRender(ApuN106::Channel &ch)
{
	uint32 phasespd = channelUse << 20;
	ch.phaseacc -= this->cycleRate;
	if(ch.phaseacc >= 0){
		if(ch.volupdate){
			ch.output = (int(tone[((ch.phase >> 18) + ch.toneadr) & 0xFF]) *ch.vol) << ChannelVolShift;
			ch.volupdate = 0;
		}
		return ch.output;
	}
	while(ch.phaseacc < 0){
		ch.phaseacc += phasespd;
		ch.phase += ch.freq;
	}
	while(ch.tonelen && (ch.phase >= ch.tonelen)){
		ch.phase -= ch.tonelen;
	}
	ch.output = (int(tone[((ch.phase >> 18) + ch.toneadr) & 0xFF]) * ch.vol) << ChannelVolShift;
	return ch.output;
}




