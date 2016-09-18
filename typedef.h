#ifndef TYPEDEF_H
#define TYPEDEF_H

//This file included NesEmu's typedef type
//Qt may have these so that programming type name with Qt.
#include <QtCore>

#define Ptr //defined this parameter is an pointer, but no effect with real memory.
#define Pure =0 //define this function are the pure virtual function

// I don't know where maybe can use it.
#define BYTE quint8
#define WORD quint16
#define BOOL qint32

#define INT qint32
#define FLOAT float

// These will use in settings normally
#define int8 qint8
#define int16 qint16
#define int32 qint32
#define int64 qint64
#define uint8 quint8
#define uint16 quint16
#define uint32 quint32
#define uint64 quint64
#define float32 float
#define real qreal

#endif // TYPEDEF_H
