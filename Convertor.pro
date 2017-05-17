TARGET = convertor
TEMPLATE = app
CONFIG += qt
QT = core gui
LIBS += -g
QMAKE_CXXFLAGS_RELEASE += -g -march=native\
    -O3
SOURCES +=convertor.cpp
