#include "apu.h"

Apu::Apu(Nes *nesParent, QObject *parent) : QObject(parent)
{
	this->exsoundSelect = 0;

	this->nes = nesParent;
	internal.setParent(nesParent);

	this->lastData = 0;
	this->lastDiff = 0;

	ZeroMemory(this->soundBuffer, sizeof(this->soundBuffer));
	ZeroMemory(this->lowpassFilter, sizeof(this->lowpassFilter));
	Clear(queue);
	Clear(exqueue);

	for(int i = 0; i < 16; i++){
		this->mute[i] = true;
	}
}

Apu::~Apu()
{

}

void Apu::soundSetup()
{
	float clock = this->nes->nesCfg->cpuClock;
	int rate = int(config.sound.rate);
	this->internal.setup(clock, rate);
	this->vrc6.setup(clock, rate);
	this->vrc7.setup(clock, rate);
	this->mmc5.setup(clock, rate);
	this->fds.setup(clock, rate);
	this->n106.setup(clock, rate);
	this->fme7.setup(clock, rate);
}

void Apu::selectExSound(quint8 data)
{
	this->exsoundSelect = data;
}

void Apu::queueClear()
{
	Clear(queue);
	Clear(exqueue);
}

void Apu::reset()
{
	Clear(queue);
	Clear(exqueue);
	this->elapsedTime = 0;

	float clock = this->nes->nesCfg->cpuClock;
	int rate = int(config.sound.rate);
	this->internal.reset(clock, rate);
	this->vrc6.reset(clock, rate);
	this->vrc7.reset(clock, rate);
	this->mmc5.reset(clock, rate);
	this->fds.reset(clock, rate);
	this->n106.reset(clock, rate);
	this->fme7.reset(clock, rate);

	this->soundSetup();
}

quint8 Apu::read(quint16 addr)
{
	return this->internal.syncRead(addr);
}

void Apu::write(quint16 addr, quint8 data)
{
	// $4018 VirtuaNES specific port
	if(addr >= 0x4000 && addr <= 0x401F){
		this->internal.syncWrite(addr, data);
		this->setQueue(this->nes->cpu->getTotalCycles(), addr, data);
	}
}

quint8 Apu::exRead(quint16 addr)
{
	uint8 data = 0;
	if(this->exsoundSelect & 0x10){
		if(addr == 0x4800){
			this->setExQueue(this->nes->cpu->getTotalCycles(), 0, 0);
		}
	}
	if(this->exsoundSelect & 0x04){
		if(addr >= 0x4040 && addr < 0x4100){
			data = this->fds.syncRead(addr);
		}
	}
	if(this->exsoundSelect & 0x08){
		if(addr >= 0x5000 && addr <= 0x5015){
			data = this->mmc5.syncRead(addr);
		}
	}
	return data;
}

void Apu::exWrite(quint16 addr, quint8 data)
{
	this->setExQueue(this->nes->cpu->getTotalCycles(), addr, data);
	if(this->exsoundSelect & 0x04){
		if(addr >= 0x4040 && addr < 0x4100){
			this->fds.syncWrite(addr, data);
		}
	}
	if(this->exsoundSelect & 0x08){
		if(addr >= 0x5000 && addr <= 0x5015){
			this->mmc5.syncWrite(addr, data);
		}
	}
}

void Apu::sync()
{

}

void Apu::syncDpcm(int cycles)
{
	this->internal.sync(cycles);
	if(this->exsoundSelect & 0x04){
		this->fds.sync(cycles);
	}
	if(this->exsoundSelect & 0x08){
		this->mmc5.sync(cycles);
	}
}

void Apu::process(quint8 *buffer, quint32 size)
{
	int bits = config.sound.bits;
	uint32 length = size / (bits / 8);
	int output;
	QueueData q;
	uint32 writetime;

	uint16 *soundBuf = this->soundBuffer;
	int count = 0;

	int filterType = config.sound.filterType;

	if(!config.sound.enable){
		FillMemory(buffer, size, (uint8)(config.sound.rate == 8? 128: 0));
		return;
	}
	/*
	 * Volume setup
	 * -->  0: Master
	 * -->  1: Rectangle 1
	 * -->  2: Rectangle 2
	 * -->  3: Triangle
	 * -->  4: Noise
	 * -->  5: DPCM
	 * -->  6: VRC6
	 * -->  7: VRC7
	 * -->  8: FDS
	 * -->  9: MMC5
	 * --> 10: N106
	 * --> 11: FME7
	 */
	int vol[24];
	bool *mute = this->mute;
	int16 *volume = config.sound.volume;
	int masterVolume = this->mute[0]? volume[0]: 0;

	// Internal
	this->vol[ 0] = mute[ 1]? (RectangleVol * volume[ 1] * masterVolume) / (10000): 0;
	this->vol[ 1] = mute[ 2]? (RectangleVol * volume[ 2] * masterVolume) / (10000): 0;
	this->vol[ 2] = mute[ 3]? (TriangleVol  * volume[ 3] * masterVolume) / (10000): 0;
	this->vol[ 3] = mute[ 4]? (NoiseVol     * volume[ 4] * masterVolume) / (10000): 0;
	this->vol[ 4] = mute[ 5]? (DpcmVol      * volume[ 5] * masterVolume) / (10000): 0;
	// VRC6
	this->vol[ 5] = mute[ 6]? (Vrc6Vol      * volume[ 6] * masterVolume) / (10000): 0;
	this->vol[ 6] = mute[ 7]? (Vrc6Vol      * volume[ 6] * masterVolume) / (10000): 0;
	this->vol[ 7] = mute[ 8]? (Vrc6Vol      * volume[ 6] * masterVolume) / (10000): 0;
	// VRC7
	this->vol[ 8] = mute[ 6]? (Vrc7Vol      * volume[ 7] * masterVolume) / (10000): 0;
	// FDS
	this->vol[ 9] = mute[ 6]? (FdsVol       * volume[ 8] * masterVolume) / (10000): 0;
	// MMC5
	this->vol[10] = mute[ 6]? (Mmc5Vol      * volume[ 9] * masterVolume) / (10000): 0;
	this->vol[11] = mute[ 7]? (Mmc5Vol      * volume[ 9] * masterVolume) / (10000): 0;
	this->vol[12] = mute[ 8]? (Mmc5Vol      * volume[ 9] * masterVolume) / (10000): 0;
	// N106
	this->vol[13] = mute[ 6]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[14] = mute[ 7]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[15] = mute[ 8]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[16] = mute[ 9]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[17] = mute[10]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[18] = mute[11]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[19] = mute[12]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	this->vol[20] = mute[13]? (N106Vol      * volume[10] * masterVolume) / (10000): 0;
	// FME7
	this->vol[21] = mute[ 6]? (Fme7Vol      * volume[11] * masterVolume) / (10000): 0;
	this->vol[22] = mute[ 7]? (Fme7Vol      * volume[11] * masterVolume) / (10000): 0;
	this->vol[23] = mute[ 8]? (Fme7Vol      * volume[11] * masterVolume) / (10000): 0;

	//double cycleRate = (double(FrameCycles) * 60.0f / 12.0f) / double(config.sound.rate);
	double cycleRate = (double(this->nes->nesCfg->frameCycles) * 60.0f / 12.0f) / double(config.sound.rate);

	// Countermeasure process of when the number of CPU cycles had looped
	// CPU saikuru suu ga ru-pu shite shimatta toki no taisaku shyori
	if(this->elapsedTime > this->nes->cpu->getTotalCycles()){
		this->queueFlush();
	}

	while(length--){
		writetime = (uint32)this->elapsedTime;
		while(getQueue(writetime, q)){
			this->writeProcess(q.addr, q.data);
		}
		while(getExQueue(writetime, q)){
			this->writeExProcess(q.addr, q.data);
		}

		// 0-4:internal 5-7:VRC6 8:VRC7 9:FDS 10-12:MMC5 13-20:N106 21-23:FME7
		output = 0;
		output += this->internal.process(0) * vol[0];
		output += this->internal.process(1) * vol[1];
		output += this->internal.process(2) * vol[2];
		output += this->internal.process(3) * vol[3];
		output += this->internal.process(4) * vol[4];
		if(this->exsoundSelect & 0x01){
			output += this->vrc6.process(0) * vol[5];
			output += this->vrc6.process(1) * vol[6];
			output += this->vrc6.process(2) * vol[7];
		}
		if(this->exsoundSelect & 0x02){
			output += this->vrc7.process(0) * vol[8];
		}
		if(this->exsoundSelect & 0x04){
			output += this-> fds.process(0) * vol[9];
		}
		if(this->exsoundSelect & 0x08){
			output += this->mmc5.process(0) * vol[10];
			output += this->mmc5.process(1) * vol[11];
			output += this->mmc5.process(2) * vol[12];
		}
		if(this->exsoundSelect & 0x10){
			output += this->n106.process(0) * vol[13];
			output += this->n106.process(1) * vol[14];
			output += this->n106.process(2) * vol[15];
			output += this->n106.process(3) * vol[16];
			output += this->n106.process(4) * vol[17];
			output += this->n106.process(5) * vol[18];
			output += this->n106.process(6) * vol[19];
			output += this->n106.process(7) * vol[20];
		}
		if(this->exsoundSelect & 0x20){
			this->fme7.process(3); // Envelope & Noise
			output += this->fme7.process(0) * vol[21];
			output += this->fme7.process(1) * vol[22];
			output += this->fme7.process(2) * vol[23];
		}
		output >>= 8;

		if(filterType == 1){
			// Low-pass filter TYPE 1 (Simple)
			output = (this->lowpassFilter[0] + output) / 2;
			this->lowpassFilter[0] = output;
		}else if(filterType == 2){
			// Low-pass filter TYPE 2 (Weighted type 1)
			output = (this->lowpassFilter[1] + this->lowpassFilter[0] + output) / 3;
			this->lowpassFilter[1] = this->lowpassFilter[0];
			this->lowpassFilter[0] = output;
		}else if(filterType == 3){
			// Low-pass fliter TYPE 3 (Weighted type 2)
			output = (this->lowpassFilter[2] + this->lowpassFilter[1] + this->lowpassFilter[0] + output) / 4;
			this->lowpassFilter[2] = this->lowpassFilter[1];
			this->lowpassFilter[1] = this->lowpassFilter[0];
			this->lowpassFilter[0] = output;
		}else if(filterType == 4){
			// Low-pass filter TYPE 4 (Weighted type 3)
			output = (this->lowpassFilter[1] + this->lowpassFilter[0] * 2 + output) / 4;
			this->lowpassFilter[1] = this->lowpassFilter[0];
			this->lowpassFilter[0] = output;
		}

#if 0
		// Cut the DC component
		{
			static double ave = 0.0f;
			static double max = 0.0f;
			static double min = 0.0f;
			double delta = (max - min) / 32768.0f;
			max -= delta;
			min += delta;
			if(output > max){
				max = output;
			}
			if(output < min){
				min = output;
			}
			ave -= ave / 1024.0f;
			ave += (max + min) / 2048.0f;
			output -= int(ave);
		}
#endif
#if 1
		// The DC component of the cut (HPF TEST)
		{
			//static double cutoff = (2.0f * 3.141592653579f * 40.0f / 44100.0f);
			static double cutofftemp = (2.0f * 3.141592653579f * 40.0f);
			double cutoff = cutofftemp / double(config.sound.rate);
			static double tmp = 0.0f;
			double in = double(output);
			double out = (in - tmp);
			tmp = tmp + cutoff * out;

			output = int(out);
		}
#endif
#if 0
		// Removal of spike noise (AGC TEST)
		{
			int diff = qabs(output - lastData);
			if(diff > 0x4000){
				output /= 4;
			}else if(diff > 0x3000){
				output /= 3;
			}else if(diff > 0x2000){
				output /= 2;
			}
			lastData = output;
		}
#endif
		// Limit
		if(output > 0x7FFF){
			output = 0x7FFF;
		}else if(output < -0x8000){
			output = -0x8000;
		}
		if(bits != 8){
			*(int16*)buffer = (int16)output;
			buffer += sizeof(int16);
		}else{
			*buffer++ = (output >> 8) ^ 0x80;
		}
		if(count < 0x0100){
			soundBuf[count++] = (uint16)output;
		}

		//elapsedtime += cycleRate
		elapsedTime += cycleRate;
	}

#if 1
	if(elapsedTime > ((this->nes->nesCfg->frameCycles / 24) + this->nes->cpu->getTotalCycles())){
		elapsedTime = this->nes->cpu->getTotalCycles();
	}
	if((elapsedTime + (this->nes->nesCfg->frameCycles / 6)) < this->nes->cpu->getTotalCycles()){
		elapsedTime = this->nes->cpu->getTotalCycles();
	}
#else
	elapsedTime = this->nes->cpu->getTotalCycles();
#endif
}

// Channel of frequency acquisition subroutine (for NSF)
int Apu::getChannelFrequency(int ch)
{
	if(!this->mute[0]){
		return 0;
	}
	// Internal
	if(no > 5){
		return this->mute[no + 1]? this->internal.getFreq(no): 0;
	}
	// VRC6
	if((this->exsoundSelect & 0x01) && no >= 0x0100 && no < 0x0103){
		return this->mute[6 + (no & 0x03)]? this->vrc6.getFreq(no & 0x03): 0;
	}
	// FDS
	if((this->exsoundSelect & 0x04) && no == 0x0300){
		return this->mute[6]? this->fds.getFreq(0): 0;
	}
	// MMC5
	if((this->exsoundSelect & 0x08) && no >= 0x0400 && no < 0x0402){
		return this->mute[6 + (no & 0x03)]? this->mmc5.getFreq(no & 0x03): 0;
	}
	// N106
	if((this->exsoundSelect & 0x10) && no >= 0x0500 && no < 0x0508){
		return this->mute[6 + (no & 0x07)]? this->n106.getFreq(no & 0x07): 0;
	}
	// FME7
	if((this->exsoundSelect & 0x20) && no >= 0x0600 && no < 0x0603){
		return this->mute[6 + (no & 0x03)]? this->fme7.getFreq(no & 0x03): 0;
	}
	// VRC7
	if((this->exsoundSelect & 0x02) && no >= 0x0700 && no < 0x0709){
		return this->mute[6]? this->vrc7.getFreq(no & 0x0F): 0;
	}
	return 0;
}

void Apu::saveState(quint8 *p)
{
#ifdef _DEBUG
	uint8 *pold = p;
#endif
	// To Flush for synchronizing the time axis
	this->queueFlush();

	this->internal.saveState(p);
	p += (this->internal.getStateSize() + 15) & (~0x0F); // padding
	// VRC6
	if(this->exsoundSelect & 0x01){
		this->vrc6.saveState(p);
		p += (this->vrc6.getStateSize() + 15) & (~0x0F); // padding
	}
	// VRC7 (not support)
	if(this->exsoundSelect & 0x02){
		this->vrc7.saveState(p);
		p += (this->vrc7.getStateSize() + 15) & (~0x0F); // padding
	}
	// FDS
	if(this->exsoundSelect & 0x04){
		this->fds.saveState(p);
		p += (this->fds.getStateSize() + 15) & (~0x0F); // padding
	}
	// MMC5
	if(this->exsoundSelect & 0x08){
		this->mmc5.saveState(p);
		p += (this->mmc5.getStateSize() + 15) & (~0x0F); //padding
	}
	// N106
	if(this->exsoundSelect & 0x10){
		this->n106.saveState(p);
		p += (this->n106.getStateSize() + 15) & (~0x0F); //padding
	}
	// FME7
	if(this->exsoundSelect & 0x20){
		this->fme7.saveState(p);
		p += (this->fme7.getStateSize() + 15) & (~0x0F); //padding
	}
#ifdef _DEBUG
	qDebug() << "Save Apu Size:" << (int)p - (int)pold;
#endif
}

void Apu::loadState(quint8 *p)
{
	// Turn off in order to synchronize the time axis
	this->queueClear();
	this->internal.loadState(p);
	p += (this->internal.getStateSize() + 15) & (~0x0F); // Padding
	// VRC6
	if(this->exsoundSelect & 0x01){
		this->vrc6.loadState(p);
		p += (this->vrc6.getStateSize() + 15) & (~0x0F); // Padding
	}
	// vrc7 (not support)
	if(this->exsoundSelect & 0x02){
		this->vrc7.loadState(p);
		p += (this->vrc7.getStateSize() + 15) & (~0x0F); // Padding
	}
	// FDS
	if(this->exsoundSelect & 0x04){
		this->fds.loadState(p);
		p += (this->fds.getStateSize() + 15) & (~0x0F); // Padding
	}
	// MMC5
	if(this->exsoundSelect & 0x08){
		this->mmc5.loadState(p);
		p += (this->mmc5.getStateSize() + 15) & (~0x0F); // Padding
	}
	// N106
	if(this->exsoundSelect & 0x10){
		this->n106.loadState(p);
		p += (this->n106.getStateSize() + 15) & (~0x0F); // Padding
	}
	// FME7
	if(this->exsoundSelect & 0x20){
		this->fme7.loadState(p);
		p += (this->fme7.getStateSize() + 15) & (~0x0F); // Padding
	}
}

void Apu::setQueue(int writetime, quint16 addr, quint8 data)
{
	this->queue.data[queue.wrPtr].time = writetime;
	this->queue.data[queue.wrPtr].addr = addr;
	this->queue.data[queue.wrPtr].data = data;
	this->queue.wrPtr++;
	this->queue.wrPtr &= QueueLength -1;
	if(this->queue.wrPtr == this->queue.rdPtr){
		qDebug() << "Queue overflow.";
	}
}

bool Apu::getQueue(int writetime, Apu::QueueData &ret)
{
	if(this->queue.wrPtr == this->queue.rdPtr){
		return false;
	}
	if(this->queue.data[queue.rdPtr].time <= writetime){
		ret = this->queue.data[this->queue.rdPtr];
		this->queue.rdPtr++;
		this->queue.rdPtr &= QueueLength -1;
		return true;
	}
	return false;
}

void Apu::setExQueue(int writetime, quint16 addr, quint8 data)
{
	this->exqueue.data[this->exqueue.wrPtr].time = writetime;
	this->exqueue.data[this->exqueue.wrPtr].addr = addr;
	this->exqueue.data[this->exqueue.wrPtr].data = data;
	this->exqueue.wrPtr++;
	this->exqueue.wrPtr &= QueueLength -1;
	if(this->exqueue.wrPtr == this->exqueue.rdPtr){
		qDebug() << "ExQueue overflow.";
	}
}

bool Apu::getExQueue(int writetime, Apu::QueueData &ret)
{
	if(this->exqueue.wrPtr == this->exqueue.rdPtr){
		return false;
	}
	if(this->exqueue.data[this->exqueue.rdPtr].time <= writetime){
		ret = this->exqueue.data[this->exqueue.rdPtr];
		this->exqueue.rdPtr++;
		this->exqueue.rdPtr &= QueueLength -1;
		return true;
	}
	return false;
}

void Apu::queueFlush()
{
	while(this->queue.wrPtr != queue.rdPtr){
		this->writeProcess(this->queue.data[this->queue.rdPtr].addr, this->queue.data[this->queue.rdPtr].data);
		this->queue.rdPtr++;
		this->queue.rdPtr &= QueueLength -1;
	}

	while(this->exqueue.wrPtr != exqueue.rdPtr){
		this->writeExProcess(this->exqueue.data[this->exqueue.rdPtr].addr, this->exqueue.data[this->exqueue.rdPtr].data);
		this->exqueue.rdPtr++;
		this->exqueue.rdPtr &= QueueLength -1;
	}
}

void Apu::writeProcess(quint16 addr, quint8 data)
{
	// $4018 VirtuaNES specific port
	if(addr >= 0x4000 && addr <= 0x401F){
		this->internal.write(addr, data);
	}
}

void Apu::writeExProcess(quint16 addr, quint8 data)
{
	if(this->exsoundSelect & 0x01){
		this->vrc6.write(addr, data);
	}
	if(this->exsoundSelect & 0x02){
		this->vrc7.write(addr, data);
	}
	if(this->exsoundSelect & 0x04){
		this->fds.write(addr, data);
	}
	if(this->exsoundSelect & 0x08){
		this->mmc5.write(addr, data);
	}
	if(this->exsoundSelect & 0x10){
		if(addr == 0x0000){
			uint8 dummy = this->n106.read(addr);
			Q_UNUSED(dummy);
		}else{
			this->n106.write(addr, data);
		}
	}
	if(this->exsoundSelect & 0x20){
		this->fme7.write(addr, data);
	}
}
