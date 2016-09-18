#ifndef NES_H
#define NES_H

#include "../typedef.h"

#define NES_PROFILER 0

//Config
#define FETCH_CYCLES 8

typedef struct NesConfig_t{
	float baseClock;		// NTSC: 21477270.0 PAL: 26601714.0
	float cpuClock;			// NTSC:  1789772.5 PAL:  1662607.125

	int totalScanLines;		// NTSC:  262       PAL:  312
	int scanLineCycles;		// NTSC: 1364       PAL: 1278

	int hDrawCycles;		// NTSC: 1024       PAL: 960
	int hBlankCycles;		// NTSC: 340        PAL: 318
	int scanLineEndCycles;	// NTSC: 4          PAL: 2

	int frameCycles;		// NTSC: 357368     PAL: 398736
	int frameIrqCycles;		// NTSC: 29830      PAL: 33252

	int frameRate;			// NTSC: 60(59.94)  PAL: 50
	float framePeriod;		// NTSC: 16.683     PAL: 20.0
} NesConfig, *lpNesConfig;

NesConfig nesConfigNtsc;
NesConfig nesConfigPal;

//NES Class
class Nes
{
public:
	// Member Value
	Cpu *cpu;
	Ppu *ppu;
	Apu *apu;
	Rom *rom;
	Pad *pad;
	Mapper *mapper;

	NesConfig *nesCfg;
public:
	Nes(const char *fname);
	virtual ~Nes();

	// Member Function
	// Emulation
	void reset();
	void softReset();

	void clock(int cycles);

	uint8 read(uint16 addr);
	void write(uint16 addr, uint8 data);

	void emulationCpu(int baseCycles);
	void emulationCpuBeforeNmi(int cycles);

	void emulateFrame(bool draw);

	// For NSF
	void emulateNsf();
	void setNsfPlay(int songNo, int songMode);
	void setNsfStop();
	bool isNsfPlaying(){return nsfPlaying; }

	// IRQ type control
	enum IrqMethod{
		IrqHSync = 0,
		IrqClock = 1,
	};

	void setIrqType(int type){ irqType = type; }
	int getIrqType() { return irqType; }

	// Frame-IRQ control (for Paris-daker rally special)
	void setFrameIrqMode(bool mode){ frameIrq = mode; }
	bool getFrameIrqMode(){ return frameIrq; }

	// NTSC/PAL
	void setVideoMode(bool mode);
	bool getVideoMode(){ return videoMode; }

	// ?
	int getDiskNo();
	void soundSetup();

	int getScanLine() { return nesScanLine; }
	bool getZapperHit() { return zapper; }
	void getZapperPos(long &x, long &y){ x = zapperX, y = zapperY; }
	void setZapperPos(long x, long y){ zapperX = x, zapperY = y; }

	// State files
	// 0: Error 1: CRC_OK -1: CRC_ERROR
	static int isStateFile(const char * fname, Rom *rom);
	bool loadState(const char *fname);
	bool saveState(const char *fname);

	int getSaveRamSize(){ return saveRamSize; }
	void setSaveRamSize(int size){ saveRamSize = size; }

	// VS-UniSystem
	uint8 getVsDipSwitch(){ return vsDipValue; }
	void setVsDipSwitch(uint8 v){ vsDipValue = v; }
	VsDipSwitch *getVsDipSwitchTable() { return vsDipTable; }

	// Snap shot
	bool snapshot();

	// For Movie
	// 0: Error 1: CRC_OK -1: CRC_ERROR
	static int isMovieFile(const char *fname, Rom *rom);

	bool isMoviePlay() { return moviePlay; }
	bool isMoveRec() { return moviePlay; }
	bool moviePlay(const char *fname);
	bool moveRec(const char *fname);
	bool movieRecAppend(const char *fname);
	bool movieStop();

	// Other control
	bool isDiskThrottle() { return diskThrottle; }
//	bool isBraking() { return brake; } //Debugger

	// Rending method
	enum RenderMethod{
		PostAllRender = 0, //After the instruction execution of the scan lines, rendering
		PreAllRender = 1, //After execution of the rendering, the execution of the instruction scan lines
		PostRender = 2, //After the instruction execution of the display period, rendering
		PreRender = 3, //After rendering execution, the execution of the instruction display period
		TileRender = 4, //Tile-based rendering
	};

	void setRenderMethod(RenderMethod type) { renderMethod = type; }
	RenderMethod getRenderMethod() { return renderMethod; }

	// Command
	enum NesCommand{
		NesCmdNone = 0,
		NesComHwReset,
		NesCmdSwReset,
		NesCmdExController,
		NesCmdDiskThrottleOn,
		NesCmdDiskThrottleOff,
		NesCmdDiskEject,
		NesCmdDisk0A,
		NesCmdDisk0B,
		NesCmdDisk1A,
		NesCmdDisk1B,
		NesCmdDisk2A,
		NesCmdDisk2B,
		NesCmdDisk3A,
		NesCmdDisk3B,
		NesCmdSoundMute, //CommandParam
	};

	void Command(NesCommand cmd);
	bool CommandParam(NesCommand cmd, int param);

	// For Movie
	void movie();
	void getMovieInfo(uint16 &recVersion, uint16 &version, uint32 recordFrames, uint32 recordTimes);

	// For Cheat
	void cheatInitial();
	bool isCheatCodeAdd();

	int getCheatCodeNum();
	bool getCheatCode(int no, CheatCode &code);
	void setCheatCodeFlag(int no, bool enable);
	void setCheatCodeAllFlag(bool enable, bool key);

	void replaceCheatCode(int no, CheatCode code);
	void addCheatCode(CheatCode code);
	void delCheatCode(int no);

	uint32 cheatRead(int length, uint16 addr);
	void cheatWrite(int length, uint16 addr, uint32 data);
	void cheatCodeProcess();

	// For Genie
	void genieIntial();
	void genieLoad(char *fname);
	void genieCodeProcess();

	// TapeDevice
	bool isTapePlay() { return tapePlay; }
	bool isTapeRec() { return tapeRec; }
	bool tapePlay(const char* fname);
	bool tapeRec(const char* fname);
	void tapeStop();
	void tape(int cycles);

	// Barcode battler(Bandai)
	void setBarcodeData(uint8 *code, int len);
	void barcode(int cycles);
	bool isBarcodeEnable() { return barcode; }
	uint8 getBarcodeStatus() { return barcodeOut; }

	// Barcode world(Sunsoft/EPOCH)
	void SetBarcode2Data(uint8 *code, int len);
	uint8 barcode2(void);
	bool isBarcode2Enable() { return barcode2; }

	// TurboFile
	void setTurboFileBank(int bank) { turboFileBank = bank; }
	int getTurboFileBank() { return turboFileBank; }

#if NES_PROFILER
	// Test area(unused)
#endif

protected:
	// Member Function
	// Emulation
	uint8 readReg(uint16 addr);
	void writeReg(uint16 addr, uint8 data);

	// State Sub
	bool readState(FILE *fp);
	void writeState(FILE *fp);

	void loadSRam();
	void saveSRam();

	void loadDisk();
	void saveDisk();

	void loadTurboFile();
	void saveTurboFile();

	// Member Variable
	int irqType;
	bool videoMode;
	bool frameIrq;

	bool zapper;
	long zapperX;
	long zapperY;

	bool padStrobe;

	RenderMethod renderMethod;

	bool diskThrottle;

	int64 baseCycles;
	int64 emulCycles;

	int nesScanLine;

	int saveRamSize;

	// For VS-Unisystem
	uint8 vsDipValue;
	VsDipSwitch *vsDipTable;

	// Snapshot number
	int snapNo;

	// For NSF
	bool nsfPlaying;
	bool nsfInit;
	int nsfSongNo;
	int nsfSongNode;

	// For Movie
	bool moviePlay;
	bool movieRec;
	uint16 movieVersion;

	FILE *moviePlay;
	MovieFileHdr hedMovie;
	uint32 movieControl;
	int32 movieStepTotal;
	int32 movieStep;
	int commandRequest;

	// For Tape
	bool tapePlay;
	bool tapeRec;
	FILE *tape;
	double tapeCycles;
	uint8 tapeIn;
	uint8 tapeOut;

	// For Barcode
	bool barcode;
	uint8 barcodeOut;
	uint8 barcodePtr;
	int barcodeCycles;
	uint8 barcodeData[256];

	// For Barcode
	bool barcode2;
	int barcode2Seq;
	int barcode2Ptr;
	int barcode2Cnt;
	uint8 barcode2Bit;
	uint8 barcode2Data[32];

	// For TurboFile
	int turboFileBank;

	// gameoption backup
	int saveRenderMethod;
	int saveIrqType;
	bool saveFrameIrq;
	bool saveVideoMode;

	// For Cheat
	bool cheatCodeAdd;
	QVector<CheatCode> cheatCode;

	// For Genie
	QVector<GenieCode> genieCode;

	// For Movie pad display
	void drawPad();
	void drawBitmap(int x, int y, uint8 *bitmap);
	static uint8 padImg[];
	static uint8 keyImg0[];
	static uint8 keyImg1[];
	static uint8 keyImg2[];

#if NES_PROFILER
	// TEST (unused)
#endif

	static uint8 font6x8[];

	void drawFont(int x, int y, uint8 chr, uint8 col);
	void drawString(int x, int y, int8 *str, uint8 col);


};

#endif // NES_H









































































