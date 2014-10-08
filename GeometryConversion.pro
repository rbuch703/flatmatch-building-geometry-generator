#-------------------------------------------------
#
# Project created by QtCreator 2014-09-22T13:16:41
#
#-------------------------------------------------

QT       = core network gui #gui is required for QColor
TARGET = GeometryConversion

CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += silent
CONFIG += debug
#CONFIG += release

TEMPLATE = app


SOURCES += \
    geometryConverter.cpp \
    polygonwithholes.cpp \
    osmtypes.cpp \
    building.cpp \
    geometry.cpp

HEADERS += \
    geometryConverter.h \
    osmtypes.h \
    polygonwithholes.h \
    buildingattributes.h \
    building.h \
    geometry.h

QMAKE_CXXFLAGS += -frounding-math #required by CGAL

LIBS += -lgmp -lCGAL -lboost_thread -lmpfr -lCGAL_Core

#QMAKE_LFLAGS += --as-needed
