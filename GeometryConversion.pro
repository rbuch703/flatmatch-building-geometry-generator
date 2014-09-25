#-------------------------------------------------
#
# Project created by QtCreator 2014-09-22T13:16:41
#
#-------------------------------------------------

QT       = core network
TARGET = GeometryConversion

CONFIG   += console
CONFIG   -= app_bundle
CONFIG   += silent
CONFIG += debug

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

LIBS += -lgmp -lCGAL
