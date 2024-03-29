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

CONFIG += console

DEFINES	+= "SIMULATION"
INCLUDEPATH += .

SOURCES += main.cpp\
    simulator.cpp \
    simulation.cpp \
    ../net_layer.c \
    llc_layer_sim.cpp

HEADERS  += \
    simulator.h \
    ../llc_layer.h \
    simulation.h \
    ../net_layer.h \
    ../mac_layer.h
