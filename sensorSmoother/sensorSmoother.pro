TARGET = sensorSmoother

SOURCES = main.cpp

include(../root.pri)

DEPENDPATH += $$INCLUDEPATH

OBJECTS_DIR = ./.tmp
MOC_DIR = ./.tmp
UI_DIR = ./.tmp
RCC_DIR = ./.tmp

QMAKE_LFLAGS -= -Wl,--as-needed

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
