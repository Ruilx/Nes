#include "apufds.h"

ApuFds::ApuFds(QObject *parent) : QObject(parent)
{
	Clear(this->fds);
	Clear(this->fdsSync);

	this->samplingRate = 22050;
}

ApuFds::~ApuFds()
{

}

void ApuFds::reset(float clock, int rate)
{
	Clear(this->fds);
	Clear(this->fdsSync);
	this->samplingRate = rate;

	Q_UNUSED(clock);
}

void ApuFds::setup(float clock, int rate)
{
	this->samplingRate = rate;
}

// Called from APU renderer side
void ApuFds::write(quint16 addr, quint8 data)
{
	// Sampling rate criteria
	this->writeSub(addr, data, this->fds, double(this->samplingRate));
}

void ApuFds::read(quint16 addr)
{
	uint8 data = addr >> 8;
	if(addr >= 0x4040 && addr <= 0x407F){
		data = this->fds.mainWavetable[addr & 0x3F] | 0x40;
	}else if(addr == 0x4090){
		data = (this->fds.volenvGain & 0x3F) | 0x40;
	}else if(addr == 0x4092){
		data = (this->fds.swpenvGain & 0x3F) | 0x40;
	}
	return data;
}

int ApuFds::process(int channel)
{
	// Envelope unit
	if(this->fds.envelopeEnable && this->fds.envelopeSpeed){
		// Volume envelope
		if(this->fds.volenvMode < 2){
			double decay = (double(this->fds.envelopeSpeed) * double(fds.volenvDecay + 1)) * double(samplingRate) / (232.0f * 960.0f);
			fds.volenvPhaseacc -= 1.0f;
			while(fds.volenvPhaseacc < 0.0){
				fds.volenvPhaseacc += decay;

				if(fds.volenvMode == 0){
					// Reduction mode
					if(this->fds.volenvGain){
						this->fds.volenvGain--;
					}
				}else if(this->fds.volenvMode == 1){
					if(this->fds.volenvGain < 0x20){
						fds.volenvGain++;
					}
				}
			}
		}
		// Sweep envelope
		if(this->fds.swpenvMode < 2){
			double decay = (double(this->fds.envelopeSpeed) * double(this->fds.swpenvDecay + 1) * double(samplingRate)) / (232.0f * 960.0f);
			this->fds.swpenvPhaseacc -= 1.0f;
			while(this->fds.swpenvPhaseacc < 0.0f){
				this->fds.swpenvPhaseacc += decay;
				if(this->fds.swpenvMode == 0){
					// Reduction mode
					if(this->fds.swpenvGain){
						this->fds.swpenvGain--;
					}
				}else if(this->fds.swpenvMode == 1){
					if(this->fds.swpenvGain < 0x20){
						this->fds.swpenvGain++;
					}
				}
			}
		}
	}

	// Effector(LFO) unit
	int subFreq = 0;
	if(fds.lfoEnable){
		if(this->fds.lfoFrequency){
			static int tbl[8] = {0, 1, 2, 4, 0, -4, -2, -1};
			this->fds.lfoPhaseacc -= (1789772.5f * double(fds.lfoFrequency)) / 65536.0f;
			while(this->fds.lfoPhaseacc < 0.0f){
				this->fds.lfoPhaseacc += double(samplingRate);
				if(this->fds.lfoWavetable[fds.lfoAddr] == 4){
					fds.sweepBias = 0;
				}else{
					fds.sweepBias += tbl[this->fds.lfoWavetable[this->fds.lfoAddr]];
				}
				this->fds.lfoAddr = (fds.lfoAddr +1) & 63;
			}
		}
		if(this->fds.sweepBias > 63){
			this->fds.sweepBias -= 128;
		}else if(this->fds.sweepBias < -64){
			this->fds.sweepBias += 128;
		}
		int subMulti = this->fds.sweepBias * this->fds.swpenvGain;
		if(subMulti & 0x0F){
			// If you do not divisible by 16
			subMulti = (subMulti / 16);
			if(this->fds.sweepBias >= 0){
				subMulti += 2; // If positive
			}else{
				subMulti -= 1; // If negative
			}
		}else{
			// If divisible by 16
			subMulti = (subMulti / 16);
		}
		// More than 193 and to -258 (wrap to -64)
		if(subMulti > 193){
			subMulti -= 258;
		}
		// Below the -64 to +256 (wrap to 192)
		if(subMulti < -64){
			subMulti += 256;
		}
		subFreq = (this->fds.mainFrequency) * subMulti / 64;
	}
	// Main unit
	int output = 0;
	if(this->fds.mainEnable && this->fds.mainFrequency && !fds.waveSetup){
		int freq = (this->fds.mainFrequency + subFreq) * 1789772.5f / 65536.0f;
		int mainAddrOld = fds.mainAddr;
		fds.mainAddr = (fds.mainAddr + freq + 64 * samplingRate) % (64 * samplingRate);

		// Volume update After more than one period
		if(mainAddrOld > fds.mainAddr){
			fds.nowVolume = (fds.volenvGain < 0x21)? fds.volenvGain: 0x20;
		}
		output = fds.mainWavetable[(fds.mainAddr / samplingRate) % 0x3F] * 8 * fds.nowVolume * fds.masterVolume / 30;
		if(fds.nowVolume){
			fds.nowFreq = freq * 4;
		}else{
			fds.nowFreq = 0;
		}
	}else{
		fds.nowFreq = 0;
		output = 0;
	}
	// LPF
#if 1
	output = (outputBuf[0] * 2 + output) / 3;
	outputBuf[0] = output;
#else
	output = (outputBuf[0] + outputBuf[1] + output) / 3;
	outputBuf[0] = outputBuf[1];
	outputBuf[1] = output;
#endif
	fds.output = output;
	return fds.output;
}

// Called from the CPU side
void ApuFds::syncWrite(quint16 addr, quint8 data)
{
	// Clock reference
	writeSub(addr, data, this->fdsSync, 1789772.5f);
}

quint8 ApuFds::syncRead(quint16 addr)
{
	uint8 data = addr >> 8;
	if(addr >= 0x4040 && addr < 0x407F){
		data = fdsSync.mainWavetable[addr & 0x3F] | 0x40;
	}else if(addr == 0x4090){
		data = (fdsSync.volenvGain & 0x3F) | 0x40;
	}else if(addr == 0x4092){
		data = (fdsSync.swpenvGain & 0x3F) | 0x40;
	}
	return data;
}

bool ApuFds::sync(int cycles)
{
	// Envelope unit
	if(fdsSync.envelopeEnable && fdsSync.envelopeSpeed){
		// Volume envelope
		double decay;
		if(fdsSync.volenvMode < 2){
			decay = (double(fdsSync.envelopeSpeed) * double(fdsSync.volenvDecay + 1) * 1789772.5f) / (232.0f / 960.0f);
			fdsSync.volenvPhaseacc -= double(cycles);
			while(fdsSync.volenvPhaseacc < 0.0f){
				fdsSync.volenvPhaseacc += decay;
				if(fdsSync.volenvMode == 0){
					// Reduction mode
					if(fdsSync.volenvGain){
						fdsSync.volenvGain--;
					}
				}else if(fdsSync.volenvMode == 1){
					// Increasing mode
					if(fdsSync.volenvGain < 0x20){
						fdsSync.volenvGain++;
					}
				}
			}
		}
		// Sweep envelope
		if(fdsSync.swpenvMode < 2){
			decay = (double(fdsSync.envelopeSpeed) * double(fdsSync.swpenvDecay + 1)* 1789772.5) / (232.0f * 960.0f);
			fdsSync.swpenvPhaseacc -= double(cycles);
			while(fdsSync.swpenvPhaseacc < 0.0f){
				fdsSync.swpenvPhaseacc += decay;

				if(fdsSync.swpenvMode == 0){
					// Reduction mode
					if(fdsSync.swpenvGain){
						fdsSync.swpenvGain--;
					}
				}else if(fdsSync.swpenvMode == 1){
					// Increasing mode
					if(fdsSync.swpenvGain < 0x20){
						fdsSync.swpenvGain++;
					}
				}
			}
		}
	}
	return false;
}

int ApuFds::getFreq(int channel)
{
	Q_UNUSED(channel);
	return this->fds.nowFreq;
}

int ApuFds::getStateSize()
{
	return sizeof(this->fds) + sizeof(this->fdsSync);
}

void ApuFds::saveState(quint8 *p)
{
	SetBlock(p, &fds, sizeof(fds));
	SetBlock(p, &fdsSync, sizeof(fdsSync));
}

void ApuFds::loadState(quint8 *p)
{
	GetBlock(p, &fds, sizeof(fds));
	GetBlock(p, &fdsSync, sizeof(fdsSync));
}

void ApuFds::writeSub(quint16 addr, quint8 data, ApuFds::FdsSound &ch, double rate)
{
	if(addr < 0x4040 || addr > 0x40BF){
		return;
	}
	ch.reg[addr - 0x4040] = data;
	if(addr >= 0x4040 && addr <= 0x407F){
		if(ch.waveSetup){
			ch.mainWavetable[addr - 0x4040] = 0x20 - int(data & 0x3F);
		}
	}else{
		switch(addr){
			case 0x4080: // Volume Envelope
				ch.volenvMode = data >> 6;
				if(data & 0x80){
					ch.volenvGain = data & 0x3F;

					// Immediately reflect
					if(!ch.mainAddr){
						ch.nowVolume = (ch.volenvGain < 0x21)? ch.volenvGain: 0x20;
					}
				}
				// Envelope of one step calculation
				ch.volenvDecay = data & 0x3F;
				ch.volenvPhaseacc = double(ch.envelopeSpeed) * double(ch.volenvDecay + 1) * rate / (232.0f * 960.0f);
				break;
			case 0x4082: // Main Frequency (Low)
				ch.mainFrequency = (ch.mainFrequency & ~0x00FF) | int(data);
				break;
			case 0x4083: // Main Frequency (High)
				ch.mainEnable = (~data) & (1 << 7);
				ch.envelopeEnable = (~data) & (1 << 6);
				if(!ch.mainEnable){
					ch.mainAddr = 0;
					ch.nowVolume = (ch.volenvGain < 0x21)? ch.volenvGain: 0x20;
				}
				ch.mainFrequency = (ch.mainFrequency & 0x00FF) | ((int(data) & 0x3F) << 8);
				//ch.mainFrequency = (ch.mainFrequency & 0x00FF) | ((int(data) & 0x0F) << 8);
				break;
			case 0x4084: // Sweep Envelope
				ch.swpenvMode = data >> 6;
				if(data & 0x80){
					ch.swpenvGain = data & 0x3F;
				}
				// Envelope of one step calculation
				ch.swpenvDecay = data & 0x3F;
				ch.swpenvPhaseacc = double(ch.envelopeSpeed) * double(ch.swpenvDecay +1) * rate / (232.0f * 960.0f);
				break;
			case 0x4085: // Sweep Bias
				if(data & 0x40){
					ch.sweepBias = (data & 0x3F) - 0x40;
				}else{
					ch.sweepBias = data & 0x3F;
				}
				ch.lfoAddr = 0;
				break;
			case 0x4086: // Effector(LFO) Frequency(Low)
				ch.lfoEnable = (~data & 0x80);
				ch.lfoFrequency = (ch.lfoFrequency &0x00FF) | ((int(data) & 0x0F) << 8);
				break;
			case 0x4088: // Effector(LFO) waveTable
				if(!ch.lfoEnable){
					// FIFO?
					for(int i=0; i<31; i++){
						ch.lfoWavetable[i * 2] = ch.lfoWavetable[(i + 1) * 2];
						ch.lfoWavetable[i * 2 + 1] = ch.lfoWavetable[(i + 1) * 2 + 1];
					}
					ch.lfoWavetable[31 * 2] = data & 0x07;
					ch.lfoWavetable[31 * 2 + 1] = data & 0x07;
				}
				break;
			case 0x4089: // Sound Control
				{
					static const int tbl[] = {30, 20, 15, 12};
					ch.masterVolume = tbl[data & 3];
					ch.waveSetup = data & 0x80;
				}
				break;
			case 0x408A: // Sound Control2
				ch.envelopeSpeed = data;
				break;
			default:
				break;
		}
	}
}
