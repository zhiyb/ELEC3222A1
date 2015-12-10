#-------------------------------------------------
#
# Project created by QtCreator 2015-12-10T14:24:00
#
#-------------------------------------------------

QT       += core gui
QT	+= network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Simulator
TEMPLATE = app

DEFINES	+= "SIMULATION"
INCLUDEPATH += .

SOURCES += main.cpp\
    simulator.cpp \
    ../llc_layer.c \
    simulation.cpp \
    mac_layer_sim.cpp

HEADERS  += \
    simulator.h \
    ../llc_layer.h \
    simulation.h \
    ../net_layer.h \
    ../mac_layer.h
