#-------------------------------------------------
#
# Project created by QtCreator 2016-06-26T14:25:46
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Nes
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    Nes/cpu.cpp \
    Nes/ips.cpp \
    Nes/mmu.cpp \
    Nes/nes.cpp \
    Nes/ApuEx/apuinternal.cpp \
    Nes/ApuEx/apuvrc6.cpp \
    Nes/ApuEx/apuvrc7.cpp \
    Nes/ApuEx/Emu2413/emu2413.cpp \
    Nes/ApuEx/apummc5.cpp \
    Nes/ApuEx/apufds.cpp \
    Nes/ApuEx/apun106.cpp \
    Nes/ApuEx/apufme7.cpp \
    Nes/apu.cpp

HEADERS  += mainwindow.h \
    typedef.h \
    Nes/cpu.h \
    Nes/cheat.h \
    Nes/ips.h \
    Nes/mmu.h \
    Nes/nes.h \
    Nes/ApuEx/apuinterface.h \
    Nes/ApuEx/apuinternal.h \
    Nes/ApuEx/apuvrc6.h \
    macro.h \
    Nes/ApuEx/apuvrc7.h \
    Nes/ApuEx/Emu2413/emu2413.h \
    Nes/ApuEx/apummc5.h \
    Nes/ApuEx/apufds.h \
    Nes/ApuEx/apun106.h \
    Nes/ApuEx/apufme7.h \
    Nes/apu.h

DISTFILES += \
    README.md
