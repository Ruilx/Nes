#include "apuvrc7.h"

ApuVrc7::ApuVrc7(QObject *parent) : QObject(parent)
{
	OPLL_init(3579545, (uint32)22050); // Temporary sampling rate
	this->Vrc7Opll = OPLL_new();
	if(this->Vrc7Opll){
		OPLL_reset(this->Vrc7Opll);
		OPLL_reset_patch(this->Vrc7Opll, OPLL_VRC7_TONE);
		this->Vrc7Opll->masterVolume = 128;
	}

	// Provisionally set
	this->reset(ApuClock, 22050);
}

ApuVrc7::~ApuVrc7()
{
	if(this->Vrc7Opll){
		OPLL_delete(this->Vrc7Opll);
		this->Vrc7Opll = NULL;
		//OPLL_close(); // Not even good (without contents)
	}
}

void ApuVrc7::reset(float clock, int rate)
{
	if(this->Vrc7Opll){
		OPLL_reset(this->Vrc7Opll);
		OPLL_reset_patch(this->Vrc7Opll, OPLL_VRC7_TONE);
		this->Vrc7Opll->masterVolume = 128;
	}
	this->address = 0;
	this->setup(clock, rate);
}

void ApuVrc7::setup(float clock, int rate)
{
	OPLL_setClock((uint32)(clock * 2.0f), (uint32)(rate));
}

void ApuVrc7::write(quint16 addr, quint8 data)
{
	if(this->Vrc7Opll){
		if(addr == 0x9010){
			address = data;
		}else if(addr == 0x9030){
			OPLL_writeReg(this->Vrc7Opll, address, data);
		}
	}
}

int ApuVrc7::process(int channel)
{
	if(this->Vrc7Opll){
		return OPLL_calc(this->Vrc7Opll);
	}
	return 0;
}

int ApuVrc7::getFreq(int channel)
{
	if(this->Vrc7Opll && channel < 8){
		int fno = ((int(this->Vrc7Opll->reg[0x20 + channel])& 0x01) << 8) + int(this->Vrc7Opll->reg[0x10 + channel]);
		int blk = (this->Vrc7Opll->reg[0x20 + channel] >> 1) & 0x07;
		float blkmul[] = {0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f};
		if(this->Vrc7Opll->reg[0x20 + channel] & 0x10){
			return int((256.0f * double(fno * blkmul[blk])) / (double(1 << 18) / (3579545.0 / 72.0)));
		}
	}
	return 0;
}




