#include "apuinternal.h"

#define RectangleVolShift 8
#define TriangleVolShift 9
#define NoiseVolShift 8
#define DpcmVolShift 8

ApuInternal::ApuInternal(QObject *parent) : QObject(parent)
{
	Clear(this->ch0);
	Clear(this->ch1);
	Clear(this->ch2);
	Clear(this->ch3);
	Clear(this->ch4);

	this->frameIrq = 0xC0;
	this->frameCycle = 0;
	this->frameIrqOccur = 0;
	this->frameCount = 0;
	this->frameType = 0;

	this->reg4015 = 0;
	this->syncReg4015 = 0;

	this->cpuClock = ApuClock;
	this->samplingRate = 22050;

	this->cycleRate = int(cpuClock * 65536.0f / 22050.0f);
}

void ApuInternal::reset(float clock, int rate)
{
	Clear(this->ch0);
	Clear(this->ch1);
	Clear(this->ch2);
	Clear(this->ch3);
	//Clear(this->ch4);

	Clear(this->toneTableEnable);
	Clear(this->toneTable);
	Clear(this->channelTone);

	this->reg4015 = 0;
	this->syncReg4015 = 0;

	// Sweep complement
	this->ch0.complement = 0x00;
	this->ch1.complement = 0xFF;

	// Noise shift register
	this->ch3.shiftReg = 0x4000;

	this->setup(clock, rate);

	// $4011 Not initialize
	uint16 addr;
	for(addr = 0x4000; addr <= 0x4010; addr++){
		this->write(addr, 0x00);
		this->syncWrite(addr, 0x00);
	}

	//this->write(0x4001, 0x08); // Reset the time will inc mode?
	//this->write(0x4005, 0x08); // Reset the time will inc mode?
	this->write(0x4012, 0x00);
	this->write(0x4013, 0x00);
	this->write(0x4015, 0x00);
	this->syncWrite(0x4012, 0x00);
	this->syncWrite(0x4013, 0x00);
	this->syncWrite(0x4015, 0x00);

	// $ 4017 is not initialized in writing
	//(for the initial mode there is a software that the hope of a 0)
	this->frameIrq = 0xC0;
	this->frameCycle = 0;
	this->frameIrqOccur = 0;
	this->frameCount = 0;
	this->frameType = 0;

	// ToneLoad
	this->toneTableLoad();
}

void ApuInternal::setup(float clock, int rate)
{
	this->cpuClock = clock;
	this->samplingRate = rate;

	this->cycleRate = int(clock * 65536.0f / float(rate));
}

// Wavetable loader
void ApuInternal::toneTableLoad()
{
	QFile *fp = new QFile();
	//char buf[512];
	QByteArray buf;

	QString tempstr;
	tempstr = CPathlib::makePathExt(this->nes->rom->getRomPath(), this->nes->getRomName(), "vtd");
	qDebug() << "Path:" << tempstr;
	fp->setFileName(tempstr);
	if(!fp->open(QIODevice::ReadOnly)){
		// View reading on the default file name
		tempstr = CPathlib::makePathExt(nes->rom->getRomPath(), "Default", "vtd");
		qDebug() << "Path:" << tempstr;
		fp->setFileName(tempStr);
		if(!fp->open(QIODevice::ReadOnly)){
			qDebug() << "File not found.";
			return;
		}
	}
	qDebug() << "Find.";

	// Read the definition file
	while(!fp->atEnd()){
		buf = fp->readLine();
		if(buf.startsWith(';') || buf.isEmpty() || buf.length() <= 0){
			continue;
		}

		QChar c = QChar(buf.at(0)).toUpper();
		if(c == QChar('@')){
/*			// Timbre read
			QByteArray pbuf = buf;
			QByteArray p;
			pbuf.remove(0, 1);
			int no;
			int val;

			// Timbre number acquisition
			no = qStr2Int(pbuf, p);
			if(p == pbuf){
				continue;
			}
			if(no < 0 || no > ToneDataMax -1){
				continue;
			}

			// Find the '='
			int indexTemp = pbuf.indexOf('=');
			if(indexTemp == -1){
				continue;
			}else{
				p = pbuf.right(pbuf.length() - indexTemp);
			}
			pbuf = p.right(p.length() -1);

			// Get the tone color data
			int i;
			for(i = 0; i < ToneDataLen; i++){
				val = qStr2Int(pbuf, p);
				if(pbuf == p){ // Acquisition failure?
					break;
				}
				if(p.at(0) == ','){ // Skip the comma ...
					pbuf = p.right(p.length() -1);
				}else{
					pbuf = p;
				}

				this->toneTable[no][i] = val;
			}
			if(i >= ToneDataMax){
				this->toneTableEnable[no] = true;
			}
*/

			// Timbre read
			char *pbuf = buf.data() + 1;
			char *p;
			int no;
			int val;
			// Timbre number acquisition
			no = ::strtol(pbuf, &p, 10);
			if(p == NULL){
				continue;
			}
			if(no < 0 || no > ToneDataMax -1){
				continue;
			}
			// Find the '='
			p = ::strchr(pbuf, '=');
			if(p == NULL){
				continue;
			}
			pbuf = p +1; // Times
			// Get the tone color data
			int i;
			for(i = 0; i < ToneDataLen; i++){
				val = ::strtol(pbuf, &p, 10);
				if(pbuf == p){ // Acquisition failure?
					break;
				}else{
					pbuf = p;
				}

				this->toneTable[no][i] = val;
			}
			if(i >= ToneDataMax){
				toneTableEnable[no] = true;
			}
		}else if(c == 'A' || c == 'B'){
			// Each channel tone color definition
			char *pbuf = buf.data() + 1;
			char *p;
			int no;
			int val;
			// Internal tone number acquisition
			no = ::strtol(pbufm &p, 10);
			if(pbuf == p){
				continue;
			}
			pbuf = p;
			if(no < 0 || no > ToneMax -1){
				continue;
			}
			// Find the '='
			p = ::strchr(pbuf, '=');
			if(p == NULL){
				continue;
			}
			pbuf = p + 1; //times
			// Timbre number acquisition
			val = ::strtol(pbuf, &p, 10);
			if(pbuf == p){
				continue;
			}
			pbuf = p;
			if(val > ToneDataMax -1){
				continue;
			}
			if(val >= 0 && toneTableEnable[val]){
				if(c == 'A'){
					this->channelTone[0][no] = val +1;
				}else{
					this->channelTone[1][no] = val +1;
				}
			}else{
				if(c == 'A'){
					this->channelTone[0][no] = 0;
				}else{
					this->channelTone[1][no] = 0;
				}
			}
		}else if(c == 'C'){
			// Each channel tone color definition
			char *pbuf = buf.data() +1;
			char *p;
			int val;
			// Find the '='
			p = ::strchr(pbuf, '=');
			if(p == NULL){
				continue;
			}
			pbuf = p +1; // times
			// Timbre number acquisition
			val = ::strtol(pbuf, &p, 10);
			if(pbuf == p){
				continue;
			}
			pbuf = p;
			if(val > ToneDataMax -1){
				continue;
			}
			if(val >= 0 && toneTableEnable[val]){
				this->channelTone[2][0] = val +1;
			}else{
				this->channelTone[2][0] = 0;
			}
		}
	}

	fp->close();
	delete fp;
}

int ApuInternal::process(int channel)
{
	switch(channel){
		case 0: return this->renderRectangle(ch0);
		case 1: return this->renderRectangle(ch1);
		case 2: return this->renderTriangle();
		case 3: return this->renderNoise();
		case 4: return this->renderDpcm();
		default: return 0;
	}
	return 0;
}

void ApuInternal::write(quint16 addr, quint8 data)
{
	switch(addr){
		// Ch0, 1 rectangle
		//case 0x4000 ... 0x4007:
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
		case 0x4004: case 0x4005: case 0x4006: case 0x4007:
			this->writeRectangle((addr < 0x4004)? 0: 1, addr, data);
			break;
		//case 0x4008 ... 0x400B:
		case 0x4008: case 0x4009: case 0x400A: case 0x400B:
			this->writeTriangle(addr, data);
			break;
		//case 0x400C ... 0x400F:
		case 0x400C: case 0x400D: case 0x400E: case 0x400F:
			this->writeNoise(addr, data);
			break;
		//case 0x4010 ... 0x4013:
		case 0x4010: case 0x4011: case 0x4012: case 0x4013:
			this->writeDpcm(addr, data);
			break;
		case 0x4015:
			this->reg4015 = data;
			if(!(data & (1 << 0))){
				this->ch0.enable = 0;
				this->ch0.lenCount = 0;
			}
			if(!(data & (1 << 1))){
				this->ch1.enable = 1;
				this->ch1.lenCount = 0;
			}
			if(!(data & (1 << 2))){
				this->ch2.enable = 0;
				this->ch2.lenCount = 0;
				this->ch2.linCount = 0;
				this->ch2.counterStart = 0;
			}
			if(!(data & (1 << 3))){
				this->ch3.enable = 0;
				this->ch3.lenCount = 0;
			}
			if(!(data & (1 << 4))){
				this->ch4.enable = 0;
				this->ch4.dmalength = 0;
			}else{
				this->ch4.enable = 0xFF;
				if(!this->ch4.dmalength){
					this->ch4.address = this->ch4.cacheAddr;
					this->ch4.dmalength = this->ch4.cacheDmalength;
					this->ch4.phaseacc = 0;
				}
			}
			break;
		case 0x4017:
			break;
		// VirtuaNES specific port
		case 0x4018:
			this->updateRectangle(this->ch0, int(data));
			this->updateRectangle(this->ch1, int(data));
			this->updateTriangle(int(data));
			this->updateNoise(int(data));
			break;
		default:
			break;
	}
}

quint8 ApuInternal::read(quint16 addr)
{
	uint8 data = addr >> 8;
	if(addr == 0x4015){
		data = 0;
		if(this->ch0.enable && this->ch0.lenCount > 0){
			data |= (1 << 0);
		}
		if(this->ch1.enable && this->ch1.lenCount > 0){
			data |= (1 << 1);
		}
		if(this->ch2.enable && this->ch2.lenCount > 0){
			data |= (1 << 2);
		}
		if(this->ch3.enable && this->ch3.lenCount > 0){
			data |= (1 << 3);
		}
	}
	return data;
}

void ApuInternal::syncWrite(quint16 addr, quint8 data)
{
	switch(addr){
		// Ch0, 1 rectangle
		//case 0x4000 ... 0x4007:
		case 0x4000: case 0x4001: case 0x4002: case 0x4003:
		case 0x4004: case 0x4005: case 0x4006: case 0x4007:
			this->syncWriteRectangle((addr < 0x4004)? 0: 1, addr, data);
			break;
		//case 0x4008 ... 0x400B:
		case 0x4008: case 0x4009: case 0x400A: case 0x400B:
			this->syncWriteTriangle(addr, data);
			break;
		//case 0x400C ... 0x400F:
		case 0x400C: case 0x400D: case 0x400E: case 0x400F:
			this->syncWriteNoise(addr, data);
			break;
		//case 0x4010 ... 0x4013:
		case 0x4010: case 0x4011: case 0x4012: case 0x4013:
			this->syncWriteDpcm(addr, data);
			break;
		case 0x4015:
			this->syncReg4015 = data;
			if(!(data & (1 << 0))){
				this->ch0.syncEnable = 0;
				this->ch0.syncLenCount = 0;
			}
			if(!(data & (1 << 1))){
				this->ch1.syncEnable = 1;
				this->ch1.syncLenCount = 0;
			}
			if(!(data & (1 << 2))){
				this->ch2.syncEnable = 0;
				this->ch2.syncLenCount = 0;
				this->ch2.syncLinCount = 0;
				this->ch2.syncCounterStart = 0;
			}
			if(!(data & (1 << 3))){
				this->ch3.syncEnable = 0;
				this->ch3.syncLenCount = 0;
			}
			if(!(data & (1 << 4))){
				this->ch4.syncEnable = 0;
				this->ch4.syncDmalength = 0;
				this->ch4.syncIrqEnable = 0;
				this->nes->cpu->clearIrq(IrqDpcm);
			}else{
				this->ch4.syncEnable = 0xFF;
				if(!this->ch4.syncDmalength){
					this->ch4.dmalength = this->ch4.syncCacheDmalength;
					this->ch4.syncCycles = 0;
				}
			}
			break;
		case 0x4017:
			this->syncWrite4017(data);
			break;
		// VirtuaNES specific port
		case 0x4018:
			this->syncUpdateRectangle(this->ch0, int(data));
			this->syncUpdateRectangle(this->ch1, int(data));
			this->syncUpdateTriangle(int(data));
			this->updateNoise(int(data));
			break;
		default:
			break;
	}
}

quint8 ApuInternal::syncRead(quint16 addr)
{
	uint8 data = addr >> 8;
	if(addr == 0x4015){
		data = 0;
		if(this->ch0.syncEnable && this->ch0.syncLenCount > 0){
			data |= (1 << 0);
		}
		if(this->ch1.syncEnable && this->ch1.syncLenCount > 0){
			data |= (1 << 1);
		}
		if(this->ch2.syncEnable && this->ch2.syncLenCount > 0){
			data |= (1 << 2);
		}
		if(this->ch3.syncEnable && this->ch3.syncLenCount > 0){
			data |= (1 << 3);
		}
		if(this->ch4.syncEnable && this->ch4.syncDmalength){
			data |= (1 << 4);
		}
		if(this->frameIrqOccur){
			data |= (1 << 5);
		}
		if(this->ch4.syncIrqEnable){
			data |= (1 << 7);
		}
		this->frameIrqOccur = 0;

		this->nes->cpu->clearIrq(IrqFrameIrq);
	}
	if(addr == 0x4017){
		if(this->frameIrqOccur){
			data = 0;
		}else{
			data |= (1 << 6);
		}
	}
	return data;
}

bool ApuInternal::sync(int cycles)
{
	this->frameCycle -= cycles * 2;
	if(this->frameCycle <= 0){
		this->frameCycle += 14915;

		this->updateFrame();
	}
}

int ApuInternal::getFreq(int channel)
{
	int freq = 0;
	// Retangle
	if(channel == 0 || channel == 1){
		Rectangle *ch;
		if(channel == 0){
			ch = &this->ch0;
		}else{
			ch = &this->ch1;
		}
		if(!ch->enable || ch->lenCount <= 0){
			return 0;
		}
		if((ch->freq < 8) || (!ch->swpInc && ch->freq > ch->freqlimit)){
			return 0;
		}
		if(!ch->volume){
			return 0;
		}
		freq = int(16.0f * this->cpuClock / float(ch->freq +1));
		return freq;
	}
	// Triangle
	if(channel == 2){
		if(!ch2.enable || ch2.lenCount <= 0){
			return 0;
		}
		if(ch2.linCount <= 0 || ch2.freq < Int2Fix(8)){
			return 0;
		}
		freq = ((int(ch2.reg[3]) & 0x07) << 8) + int(ch2.reg[2]) +1;
		freq = int(8.0f * this->cpuClock / float(freq));
		return freq;
	}
	// Noise
	if(channel == 3){
		if(!ch3.enable || ch3.lenCount <= 0){
			return 0;
		}
		if(ch3.envFixed){
			if(!ch3.volume){
				return 0;
			}
		}else{
			if(!ch3.envVol){
				return 0;
			}
		}
		return 1;
	}
	// DPCM
	if(channel == 4){
		if(ch4.enable && ch4.dmalength){
			return 1;
		}
	}
	return 0;
}

int ApuInternal::getStateSize()
{
	return 4 * sizeof(uint8) + 3 * sizeof(int) + sizeof(ch0) + sizeof(ch1) +
			sizeof(ch2) + sizeof(ch3) + sizeof(ch4);
}

void ApuInternal::saveState(quint8 *p)
{
	SetByte(p, reg4015);
	SetByte(p, syncReg4015);

	SetInt(p, frameCycle);
	SetInt(p, frameCount);
	SetInt(p, frameType);
	SetByte(p, frameIrq);
	SetByte(p, frameIrqOccur);

	SetBlock(p, &ch0, sizeof(ch0));
	SetBlock(p, &ch1, sizeof(ch1));
	SetBlock(p, &ch2, sizeof(ch2));
	SetBlock(p, &ch3, sizeof(ch3));
	SetBlock(p, &ch4, sizeof(ch4));
}

void ApuInternal::loadState(quint8 *p)
{
	GetByte(p, reg4015);
	GetByte(p, syncReg4015);

	GetInt(p, frameCycle);
	GetInt(p, frameCount);
	GetInt(p, frameType);
	GetByte(p, frameIrq);
	GetByte(p, frameIrqOccur);

	GetBlock(p, &ch0, sizeof(ch0));
	GetBlock(p, &ch1, sizeof(ch1));
	GetBlock(p, &ch2, sizeof(ch2));
	GetBlock(p, &ch3, sizeof(ch3));
	// p += sizeof(ch3);
	GetBlock(p, &ch4, sizeof(ch4));
}

// $4017 Write
void ApuInternal::syncWrite4017(quint8 data)
{
	this->frameCycle = 0;
	this->frameIrq = data;
	this->frameIrqOccur = 0;

	this->nes->cpu->clearIrq(IrqFrameIrq);

	this->frameType = (data & 0x80)? 1: 0;
	this->frameCount = 0;
	if(data & 0x80){
		this->updateFrame();
	}
	this->frameCount = 1;
	this->frameCycle = 14915;
}

void ApuInternal::updateFrame()
{
	if(!this->frameCount){
		if(!(this->frameIrq & 0xC0) && this->nes->getFrameIrqMode()){
			this->frameIrqOccur = 0xFF;
			this->nes->cpu->setIrq(IrqFrameIrq);
		}
	}
	if(this->frameCount == 3){
		if(this->frameIrq & 0x80){
			this->frameCycle += 14915;
		}
	}
	//Counters Update
	this->nes->write(0x4018, (uint8)(this->frameCount));
	this->frameCount = (this->frameCount +1) & 3;
}

void ApuInternal::writeRectangle(int no, quint16 addr, quint8 data)
{
	Rectangle ch = (no==0)? ch0: ch1;

	ch.reg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch.holdnote = data & 0x20;
			ch.volume = data & 0x0F;
			ch.envFixed = data & 0x10;
			ch.envDecay = (data & 0x0F) +1;
			ch.duty = dutyLut[data >> 6];
			break;
		case 1:
			ch.swpOn = data & 0x80;
			ch.swpInc = data & 0x08;
			ch.swpShift = data & 0x07;
			ch.swpDecay = ((data >> 4) & 0x07) +1;
			ch.freqlimit = this->freqLimit[data & 0x07];
			break;
		case 2:
			ch.freq = (ch.freq & (~0xFF)) + data;
			break;
		case 3: //Master
			ch.freq = ((data & 0x07) << 8) + (ch.freq & 0xFF);
			ch.lenCount = this->vblLength[data >> 3] *2;
			ch.envVol = 0x0F;
			ch.envCount = ch.envDecay +1;
			ch.adder = 0;
			if(this->reg4015 & (1 << no)){
				ch.enable = 0xFF;
			}
			break;
	}
}

void ApuInternal::updateRectangle(ApuInternal::Rectangle &ch, int type)
{
	if(!ch.enable || ch.lenCount <= 0){
		return;
	}
	// Update Length/Sweep
	if(!(type & 1)){
		// Update Length
		if(ch.lenCount && !ch.holdnote){
			// Holdnote
			if(ch.lenCount){
				ch.lenCount--;
			}
		}
	}

	// Update Sweep
	if(ch.swpOn && ch.swpShift){
		if(ch.swpCount){
			ch.swpCount--;
		}
		if(ch.swpCount == 0){
			ch.swpCount = ch.swpDecay;
			if(ch.swpInc){
				// Sweep increment(to higher frequency)
				if(!ch.complement){
					ch.freq += ~(ch.freq >> ch.swpShift); // CH0
				}else{
					ch.freq -= (ch.freq >> ch.swpShift); // CH1
				}else{
					//Sweep decrement(to lower frequency)
					ch.freq += (ch.freq >> ch.swpShift);
				}
			}
		}
	}
	// Update Envelope
	if(ch.envCount){
		ch.envCount--;
	}
	if(ch.envCount == 0){
		ch.envCount = ch.envDecay;

		// Holdnote
		if(ch.holdnote){
			ch.envVol = (ch.envVol -1) & 0x0F;
		}else if(ch.envVol){
			ch.envVol--;
		}
	}
	if(!ch.envFixed){
		ch.nowvolume = ch.envVol << RectangleVolShift;
	}
}

// Render Rectangle
int ApuInternal::renderRectangle(ApuInternal::Rectangle &ch)
{
	if(!ch.enable || ch.lenCount <= 0){
		return 0;
	}
	// Channel disable?
	if((ch.freq < 8) || (!ch.swpInc && ch.freq > ch.freqlimit)){
		return 0;
	}

	if(ch.envFixed){
		ch.nowvolume = ch.volume << RectangleVolShift;
	}
	int volume = ch.nowvolume;

	if(!(Config.sound.changeTone && this->channelTone[(!ch.complement)? 0: 1][ch.reg[0] << 6])){
		// Interpolation processing
		// Hokan Shori
		double total;
		double sampleWeight = ch.phaseacc;
		if(sampleWeight > cycleRate){
			sampleWeight = cycleRate;
		}
		total = (ch.adder < ch.duty)? sampleWeight: -sampleWeight;

		int freq = Int2Fix(ch.freq+1);
		ch.phaseacc -= cycleRate;
		while(ch.phaseacc < 0){
			ch.phaseacc += freq;
			ch.adder = (ch.adder +1) & 0x0F;

			sampleWeight = freq;
			if(ch.phaseacc > 0){
				sampleWeight -= ch.phaseacc;
			}
			total += (ch.adder < ch.duty)? sampleWeight: -sampleWeight;
		}
		return int(qFloor(volume * total / this->cycleRate + 0.5));
	}else{
		int *pTone = this->toneTable[this->channelTone[(!ch.complement)? 0: 1][ch.reg[0] >> 6] -1];

		// No update
		// Koushin nashi
		ch.phaseacc -= this->cycleRate *2;
		if(ch.phaseacc >= 0){
			return pTone[ch.adder & 0x1F] * volume / ((1 << RectangleVolShift) /2);
		}

		// Only one step update
		int freq = Int2Fix(ch.freq +1);
		if(freq > this->cycleRate *2){
			ch.phaseacc += freq;
			ch.adder = (ch.adder +1) & 0x1F;
			return pTone[ch.adder & 0x1F] * volume / ((1 << RectangleVolShift) /2);
		}

		// weighted average
		// Kajyuu heikin

		int numTimes = 0;
		int total = 0;
		while(ch.phaseacc < 0){
			ch.phaseacc += freq;
			ch.adder = (ch.adder +1) & 0x1F;
			total += pTone[ch.adder & 0x1F] * volume / ((1 << RectangleVolShift) /2);
			numTimes++;
		}
		return total / numTimes;
	}
}

// Sync Write Rectangle
void ApuInternal::syncWriteRectangle(int no, quint16 addr, quint8 data)
{
	Rectangle &ch = (no==0)? ch0: ch1;

	ch0.syncReg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch.syncHoldnote = data & 0x20;
			break;
		case 1: case 2:
			break;
		case 3:
			// Master
			ch.syncLenCount = this->vblLength[data >> 3] *2;
			if(this->syncReg4015 & (1 << no)){
				ch.syncEnable = 0xFF;
			}
			break;
	}
}

// Sync Update Rectangle
void ApuInternal::syncUpdateRectangle(ApuInternal::Rectangle &ch, int type)
{
	if(!ch.syncEnable || ch.syncLenCount <= 0){
		return;
	}

	// Update Length
	if(ch.syncLenCount && !ch.syncHoldnote){
		if(!(type & 1) && ch.syncLenCount){
			ch.syncLenCount--;
		}
	}
}

// Write Triangle
void ApuInternal::writeTriangle(quint16 addr, quint8 data)
{
	ch2.reg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch2.holdnote = data & 0x80;
			break;
		case 1: // Unused
			break;
		case 2:
			ch2.freq = Int2Fix(((int(ch2.reg[3]) & 0x07) << 8) + int(data) +1);
			break;
		case 3: // Master
			ch2.freq = Int2Fix(((int(data) & 0x07) << 8) + int(ch2.reg[2]) +1);
			ch2.lenCount = this->vblLength[data >> 3] *2;
			ch2.counterStart = 0x80;

			if(this->reg4015 & (1 << 2)){
				ch2.enable = 0xFF;
			}
			break;
	}
}

// Update Triangle
void ApuInternal::updateTriangle(int type)
{
	if(!ch2.enable){
		return;
	}
	if(!(type & 0x01) && !ch2.holdnote){
		if(ch2.lenCount){
			ch2.lenCount--;
		}
	}
	// Update Length/Linear
	if(ch2.counterStart){
		ch2.linCount = ch2.reg[0] & 0x7F;
	}else if(ch2.linCount){
		ch2.linCount--;
	}
	if(!ch2.holdnote && ch2.linCount){
		ch2.counterStart = 0;
	}
}

// Render Triangle
int ApuInternal::renderTriangle()
{
	int vol;
	if(Config.sound.disableVolumeEffect){
		vol = 256;
	}else{
		vol = 256 - int((ch4.reg[1] & 0x01) + ch4.dpcmValue *2);
	}
	if(!ch2.enable || (ch2.lenCount <= 0) || (ch2.linCount <= 0)){
		return ch2.nowvolume * vol / 256;
	}
	if(ch2.freq < Int2Fix(8)){
		return ch2.nowvolume * vol / 256;
	}
	if(!(Config.sound.changeTone && this->channelTone[2][0])){
		ch2.phaseacc -= this->cycleRate;
		if(ch2.phaseacc >= 0){
			return ch2.nowvolume * vol / 256;
		}
		if(ch2.freq > this->cycleRate){
			ch2.phaseacc += ch2.freq;
			ch2.adder = (ch2.adder +1) & 0x1F;

			if(ch2.adder < 0x10){
				ch2.nowvolume = (ch2.adder & 0x0F) << TriangleVolShift;
			}else{
				ch2.nowvolume = (0x0F - (ch2.adder & 0x0F)) << TriangleVolShift;
			}
			return ch2.nowvolume * vol / 256;
		}
		// weighted average
		int numTimes = 0;
		int total = 0;
		while(ch2.phaseacc < 0){
			ch2.phaseacc += ch2.freq;
			ch2.adder = (ch2.adder +1) & 0x1F;

			if(ch2.adder < 0x10){
				ch2.nowvolume = (ch2.adder & 0x0F) << TriangleVolShift;
			}else{
				ch2.nowvolume = (0x0F - (ch2.adder & 0x0F)) << TriangleVolShift;
			}
			total += ch2.nowvolume;
			numTimes++;
		}
		return (total / numTimes) * vol / 256;
	}else{
		int *tone = this->toneTable[this->channelTone[2][0] -1];
		ch2.phaseacc -= cycleRate;
		if(ch2.phaseacc >= 0){
			return ch2.nowvolume * vol / 256;
		}
		if(ch2.freq > this->cycleRate){
			ch2.phaseacc += ch2.freq;
			ch2.adder = (ch2.adder +1) & 0x1F;
			ch2.nowvolume = tone[ch2.adder & 0x1F] * 0x0F;
			return ch2.nowvolume * vol / 256;
		}
		// weighted average
		int numTimes = 0;
		int total = 0;
		while(ch2.phaseacc < 0){
			ch2.phaseacc += ch2.freq;
			ch2.adder = (ch2.adder +1) & 0x1F;
			total += tone[ch2.adder & 0x1F] * 0x0F;
			numTimes++;
		}
		return (total / numTimes) * vol / 256;
	}
}

// Sync Write Triangle
void ApuInternal::syncWriteTriangle(qint16 addr, quint8 data)
{
	ch2.syncReg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch2.syncHoldnote = data & 0x80;
			break;
		case 1:
			break;
		case 2:
			break;
		case 3: // Master
			ch2.syncLenCount = this->vblLength[ch2.syncReg[3] >> 3] *2;
			ch2.syncCounterStart = 0x80;

			if(this->syncReg4015 & (1 << 2)){
				ch2.syncEnable = 0xFF;
			}
			break;
	}
}

// Sync Update Triangle
void ApuInternal::syncUpdateTriangle(int type)
{
	if(ch2.syncEnable){
		return;
	}
	if(!(type & 0x01) && !ch2.syncHoldnote){
		if(ch2.syncLenCount){
			ch2.syncLenCount--;
		}
	}
	// Update Length/Linear
	if(ch2.syncCounterStart){
		ch2.syncLinCount = ch2.syncReg[0] & 0x7F;
	}else if(ch2.syncLinCount){
		ch2.syncLinCount--;
	}
	if(!ch2.syncHoldnote && ch2.syncLinCount){
		ch2.syncCounterStart = 0;
	}
}

void ApuInternal::writeNoise(quint16 addr, quint8 data)
{
	ch3.reg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch3.holdnote = data & 0x20;
			ch3.volume = data & 0x0F;
			ch3.envFixed = data & 0x10;
			ch3.envDecay = (data & 0x0F) +1;
			break;
		case 1: // Unused
			break;
		case 2:
			ch3.freq = Int2Fix(this->noiseFreq[data & 0x0F]);
			ch3.xorTap = (data & 0x80)? 0x40: 0x02;
			break;
		case 3: // Master
			ch3.lenCount = this->vblLength[data >> 3] *2;
			ch3.envVol = 0x0F;
			ch3.envCount = ch3.envDecay +1;

			if(this->reg4015 & (1 << 3)){
				ch3.enable = 0xFF;
			}
			break;
	}
}

void ApuInternal::updateNoise(int type)
{
	if(!ch3.enable || ch3.lenCount <= 0){
		return;
	}
	// Update Length
	if(!ch3.holdnote){
		//Holdnote
		if(!(type & 0x01) && ch3.lenCount){
			ch3.lenCount--;
		}
	}
	// Update Envelope
	if(ch3.envCount){
		ch3.envCount--;
	}
	if(ch3.envCount == 0){
		ch3.envCount = ch3.envDecay;

		// Holdnote
		if(ch3.holdnote){
			ch3.envVol = (ch3.envVol -1) & 0x0F;
		}else if(ch3.envVol){
			ch3.envVol--;
		}
	}
	if(!ch3.envFixed){
		ch3.nowvolume = ch3.envVol << RectangleVolShift;
	}
}

// Noise ShiftRegister
quint8 ApuInternal::noiseShiftreg(quint8 xorTap)
{
	int bit0 = ch3.shiftReg & 1;
	int bit14;

	if(ch3.shiftReg & xorTap){
		bit14 = bit0 ^ 1;
	}else{
		bit14 = bit0 ^ 0;
	}
	ch3.shiftReg >>= 1;
	ch3.shiftReg |= (bit14 << 14);
	return bit0 ^ 1;
}

int ApuInternal::renderNoise()
{
	if(!ch3.enable || ch3.lenCount <= 0){
		return 0;
	}
	if(ch3.envFixed){
		ch3.nowvolume = ch3.volume << RectangleVolShift;
	}
	int vol = 256 - int((ch4.reg[1] & 0x01) + ch4.dpcmValue * 2);
	ch3.phaseacc -= this->cycleRate;
	if(ch3.phaseacc >= 0){
		return ch3.output * vol / 256;
	}
	if(ch3.freq > this->cycleRate){
		ch3.phaseacc += ch3.freq;
		if(noiseShiftreg(ch3.xorTap)){
			ch3.output = ch3.nowvolume;
		}else{
			ch3.output = -ch3.nowvolume;
		}
		return ch3.output * vol / 256;
	}
	int numTimes = 0;
	int total = 0;
	while(ch3.phaseacc < 0){
		ch3.phaseacc += ch3.freq;
		if(noiseShiftreg(ch3.xorTap)){
			ch3.output = ch3.nowvolume;
		}else{
			ch3.output = -ch3.nowvolume;
		}
		total += ch3.output;
		numTimes++;
	}
	return (total / numTimes) * vol / 256;
}

// Sync Write Noise
void ApuInternal::syncWriteNoise(quint16 addr, quint8 data)
{
	ch3.syncReg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch3.syncHoldnote = data & 0x20;
			break;
		case 1:
			break;
		case 2:
			break;
		case 3: // Master
			ch3.syncLenCount = this->vblLength[data >> 3] *2;
			if(this->reg4015 & (1 << 3)){
				ch3.syncEnable = 0xFF;
			}
			break;
	}
}

// Sync Update Noise
void ApuInternal::syncUpdateNoise(int type)
{
	if(!ch3.syncEnable || ch3.syncLenCount <= 0){
		return;
	}
	// Update length
	if(ch3.syncLenCount && !ch3.syncHoldnote){
		if(!(type & 0x01) && ch3.syncLenCount){
			ch3.syncLenCount--;
		}
	}
}

void ApuInternal::writeDpcm(quint16 addr, quint8 data)
{
	ch4.reg[addr & 0x03] = data;
	switch(addr & 0x03){
		case 0:
			ch4.freq = Int2Fix(nes->getVideoMode()? this->dpcmCyclesPal[data & 0x0F]: this->dpcmCycles[data & 0x0F]);
			ch4.looping = data & 0x40;
			break;
		case 1:
			ch4.dpcmValue = (data & 0x7F) >> 1;
			break;
		case 2:
			ch4.cacheAddr = 0xC000 + (uint16)(data << 6);
			break;
		case 3:
			ch4.cacheDmalength = ((data << 4) +1) << 3;
			break;
	}
}

int ApuInternal::renderDpcm()
{
	if(ch4.dmalength){
		ch4.phaseacc -= this->cycleRate;

		while(ch4.phaseacc < 0){
			ch4.phaseacc += ch4.freq;
			if(!(ch4.curByte & 7)){
				ch4.curByte = nes->read(ch4.address);
				if(ch4.address == 0xFFFF){
					ch4.address = 0x8000;
				}else{
					ch4.address++;
				}
			}
			if(!(--ch4.dmalength)){
				if(ch4.looping){
					ch4.address = ch4.cacheAddr;
					ch4.dmalength = ch4.cacheDmalength;
				}else{
					ch4.enable = 0;
					break;
				}
			}
			// positive data
			if(ch4.curByte & (1 << ((ch4.dmalength & 7) ^ 7))){
				if(ch4.dpcmValue < 0x3F){
					ch4.dpcmValue -= 1;
				}
			}
		}
	}
#if 1
	// Inchiki kusai puchinoizukatto
	ch4.dpcmOutputReal = int((ch4.reg[1] & 0x01) + ch4.dpcmValue *2) - 0x40;
	if(qAbs(ch4.dpcmOutputReal - ch4.dpcmOutputFake) <= 8){
		ch4.dpcmOutputFake = ch4.dpcmOutputReal;
		ch4.output = int(ch4.dpcmOutputReal) << DpcmVolShift;
	}else{
		if(ch4.dpcmOutputReal > ch4.dpcmOutputFake){
			ch4.dpcmOutputFake += 8;
		}else{
			ch4.dpcmOutputFake -= 8;
		}
		ch4.output = int(ch4.dpcmOutputFake) << DpcmVolShift;
	}
#else
	ch4.output = ((int(ch4.reg[1]) & 0x01) + int(ch4.dpcmValue) *2) << DpcmVolShift;
//	ch4.output = ((int(ch4.reg[1]) & 0x01) + int(ch4.dpcmValue) *2) - 0x40 << DpcmVolShift;
#endif
	return ch4.output;
}

void ApuInternal::syncWriteDpcm(quint16 addr, quint8 data)
{
	ch4.reg[addr & 0x03] = data;
	switch(addr & 3){
		case 0:
			ch4.syncCacheCycles = this->nes->getVideoMode()? this->dpcmCyclesPal[data & 0x0F] *8: this->dpcmCycles[data & 0x0F] *8;
			ch4.syncLooping = data & 0x40;
			ch4.syncIrqGen = data & 0x80;
			if(!ch4.syncIrqGen){
				ch4.syncIrqEnable = 0;
				this->nes->cpu->clearIrq(IrqDpcm);
			}
			break;
		case 1:
			break;
		case 2:
			break;
		case 3:
			ch4.syncCacheDmalength = (data << 4) +1;
			break;
	}
}

bool ApuInternal::syncUpdateDpcm(int cycles)
{
	bool irq = false;
	if(ch4.syncEnable){
		ch4.syncCycles -= cycles;
		while(ch4.syncCycles < 0){
			ch4.sync_cycles += ch4.syncCacheCycles;
			if(ch4.syncDmalength){
				if(--ch4.syncDmalength < 2){
					if(ch4.syncLooping){
						ch4.syncDmalength = ch4.syncCacheDmalength;
					}else{
						ch4.syncDmalength = 0;
						if(ch4.syncIrqGen){
							ch4.syncIrqEnable = 0xFF;
							this->nes->cpu->setIrq(IrqDpcm);
						}
					}
				}
			}
		}
	}
	if(ch4.syncIrqEnable){
		irq = true;
	}
	return irq;
}
