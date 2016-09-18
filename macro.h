#ifndef MACRO_H
#define MACRO_H

//This file saved any deleted or freed parameter packed by definations
//Some base opeations completed by Qt-Objects so that may clean this file
//in order to use it out of Qt.
#include <QtCore>

/*
 * .*Memory Instructions:
 * This marco was defined in winnt.h(MSVC 12.0)(Rtl.*Memory Marco)(Line: 17772 start)
 * Also defined in minwinbase.h(MSVC 12.0)(.*Memory Marco)(Line: 35 start)
 * d: Destination
 * s: Source
 * l: Length
 * f: Fill
 */
#ifndef ZeroMemory
#	define ZeroMemory(d,l) (memset((d), 0, (l)))
#	define Clear(p) ZeroMemory(&(p),sizeof(p))
#endif
#ifndef EqualMemory
#	define EqualMemory(d,s,l) (!memcmp((d), (s), (l)))
#endif
#ifndef MoveMemory
#	define MoveMemory(d,s,l) (memmove((d), (s), (l)))
#endif
#ifndef CopyMemory
#	define CopyMemory(d,s,l) (memcopy((d), (s), (l)))
#endif
#ifndef FillMemory
#	define FillMemory(d,l,f) (memset((d), (f), (l)))
#endif

int	qStr2Int(QString string, QString &rest, int base = 10, bool *ok = nullptr){
	int numberLength = string.indexOf(QRegExp("[^0-9\\-]"), 0);
	QString temp = string;
	temp.truncate(numberLength);
	rest = string.right(string.length() - numberLength);
	return temp.toInt(ok, base);
}

int qStr2Int(QByteArray string, QByteArray &rest, int base = 10, bool *ok = nullptr){
	QString restTemp;
	int result = qStr2Int(QString(string), restTemp, base, ok);
	rest = restTemp.toLocal8Bit();
	return result;
}

#endif // MACRO_H
