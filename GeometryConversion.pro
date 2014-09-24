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

TEMPLATE = app


SOURCES += main.cpp \
    geometryConverter.cpp \
    polygonwithholes.cpp \
    osmtypes.cpp

HEADERS += \
    geometryConverter.h \
    osmtypes.h \
    polygonwithholes.h \
    buildingattributes.h
