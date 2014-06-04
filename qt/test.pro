QT += core gui widgets testlib

TARGET = test

SOURCES += \
    ../test/main.cpp \
    ../src/datum/datum.cpp \
    ../src/datum/eval.cpp \
    ../src/datum/float.cpp \
    ../test/test_datum.cpp \

HEADERS  += \
    ../src/datum/datum.h \
    ../src/datum/eval.h \
    ../src/datum/float.h \
    ../test/test_datum.h

INCLUDEPATH += ../src

CONFIG += c++11

macx {
    QMAKE_CXXFLAGS += $$system(/usr/local/bin/python3-config --includes)
    QMAKE_LFLAGS   += $$system(/usr/local/bin/python3-config --ldflags)
}
