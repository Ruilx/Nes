#ifndef CHEAT_H
#define CHEAT_H

#include <QtCore>

#define CheatEnable (1 << 0)
#define CheatKeyDisable (1 << 1)

enum CheatType{
	CheatTypeAlways = 0, //Always writing
	CheatTypeOnce = 1, //Writing only once
	CheatTypeGreater = 2, //when data is larger than setting
	CheatTypeLess = 3, // when data is small than setting
};

enum DataLength{
	CheatLength1Byte = 0,
	CheatLength2Byte = 1,
	CheatLength3Byte = 2,
	CheatLength4Byte = 3,
};

class CheatCode{
public:
	quint8 enable;
	CheatType type;
	DataLength length;
	quint16 address;
	quint32 data;

	QString comment;
};

class GenCode{
public:
	quint16 address;
	quint8 data;
	quint8 cmp;
};

#endif // CHEAT_H
