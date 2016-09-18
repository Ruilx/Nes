#ifndef APUINTERFACE_H
#define APUINTERFACE_H

#include "../../macro.h"
#include "../../typedef.h"

#define ApuClock 1789772.5f

// Fixed point decimal marco
#define Int2Fix(x) ((x) << 16)
#define Fix2Int(x) ((x) >> 16)

// APU interface abstract class
class ApuInterface{
public:
	// Which always require authentication
	virtual void reset(float clock, int rate) Pure;
	virtual void setup(float clock, int rate) Pure;
	virtual void write(uint16 addr, uint8 data) Pure;
	virtual int process(int channel) Pure;
	
	// Implemented in options
	virtual uint8 read(uint16 addr) { return uint8(addr >> 8); }
	
	virtual void writeSync(uint16 addr, uint8 data) { Q_UNUSED(addr), Q_UNUSED(data); }
	virtual uint8 readSync(uint16 addr) { Q_UNUSED(addr); return 0; }
	virtual void vSync() {}
	virtual bool sync(int cycles) { Q_UNUSED(cycles); return false; } // return true when generating the IRQ
	
	virtual int getFreq(int channel) { Q_UNUSED(channel); return 0; }
	
	// For state save
	virtual int getStateSize() { return 0; }
	virtual void saveState(uint8 *p) { Q_UNUSED(p); }
	virtual void loadState(uint8 *p) { Q_UNUSED(p); }
};

#endif // APUINTERFACE_H
