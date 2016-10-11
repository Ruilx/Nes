#ifndef MMU_H
#define MMU_H

#include <QtCore>
#include "../typedef.h"
#include "../macro.h"

class Mmu : public QObject
{
	Q_OBJECT
public:
	//CPU memory bank
	Ptr quint8 cpuMemBank[8]; //8k unit
	quint8 cpuMemType[8];
	qint32 cpuMemPage[8];  //State for save

	//PPU memory bank
	Ptr quint8 ppuMemBank[12]; //1k unit
	quint8 ppuMemType[12];
	qint32 ppuMemPage[12]; //State for save
	quint8 cramUsed[16];   //State for save

	//NES memory
	quint8 ram [  8*1024]; //NES internal organs RAM
	quint8 wram[128*1024]; //Work/Backup RAM
	quint8 dram[ 40*1024]; //Disk system RAM
	quint8 xram[  8*1024]; //Dummy bank
	quint8 eram[ 32*1024]; //RAM for extended equipment

	quint8 cram[ 32*1024]; //Character pattern RAM
	quint8 vram[  4*1024]; //Name table / attribute RAM

	quint8 spram[0x100];   //Sprite RAM
	quint8 bgpal[0x10];    //BG palette
	quint8 sppal[0x10];    //SP palette

	//Register
	quint8 cpuReg[0x18];   //NES $4000 - $4017
	quint8 ppuReg[0x04];   //NES $2000 - $2003

	//Frame-IRQ Register
	quint8 frameIrq;       //NES $4017

	//PPU internal register
	quint8 ppu56Toggle;    //$2005 - $2006 Toggle
	quint8 ppu7Temp;       //$2007 read buffer
	quint16 loopyT;        //Same as $2005 / $2006
	quint16 loopyV;        //Same as $2005 / $2006
	quint16 loopyX;        //Tile X offset

	//ROM data pointer
	Ptr quint8 *prom;       //PROM ptr
	Ptr quint8 *vrom;       //VROM ptr

	#ifdef _DataTrace
	//For dis...
	Ptr quint8 *promAccress;
	#endif

	//ROM bank size
	qint32 prom8kSize;
	qint32 prom16kSize;
	qint32 prom32kSize;
	qint32 vrom1kSize;
	qint32 vrom2kSize;
	qint32 vrom4kSize;
	qint32 vrom8kSize;

	//Function
	explicit Mmu(QObject *parent = 0);
	void nesSubMemoryInitial();
	void setPromBank(uint8 page, uint8 *ptr, uint8 type);
	void setProm8KBank(uint8 page, int bank);
	void setProm16KBank(uint8 page, int bank);
	void setProm32KBank(int bank);
	void setProm32KBank(int bank0, int bank1, int bank2, int bank3);
	void setVromBank(uint8 page, uint8 *ptr, uint8 type);
	void setVrom1KBank(uint8 page, int bank);
	void setVrom2KBank(uint8 page, int bank);
	void setVrom4KBank(uint8 page, int bank);
	void setVrom8KBank(int bank);
	void setVrom8KBank(int bank0, int bank1, int bank2, int bank3, int bank4, int bank5, int bank6, int bank7);
	void setCram1KBank(uint8 page, int bank);
	void setCram2KBank(uint8 page, int bank);
	void setCram4KBank(uint8 page, int bank);
	void setCram8KBank(int bank);
	void setVRam1KBank(uint8 page, int bank);
	void setVramBank(int bank0, int bank1, int bank2, int bank3);
	void setVramMirror(int type);
	void setVramMirror(int bank0, int bank1, int bank2, int bank3);

	// Memory Type
	// For PROM (CPU)
#define BankTypeRom    0x00
#define BankTypeRam    0xFF
#define BankTypeDram   0x01
#define BanktypeMapper 0x80
	// For VROM/VRAM/CRAM (PPU)
#define BankTypeVrom   0x00
#define BankTypeCram   0x01
#define BankTypeVram   0x80
	// Mirror Type
#define VramHMirror    0x00 // Horizontal
#define VramVMirror    0x01 // Virtical
#define VramMirror4    0x02 // All screen
#define VramMirror4L   0x03 // Pa10 L locked $2000-$23FF's mirror
#define VramMirror4H   0x04 // PA10 H locked $2400-$27FF's mirror

signals:

public slots:
};

#endif // MMU_H























