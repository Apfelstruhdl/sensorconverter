TARGET = sensorconverter

include(root.pri)

INCLUDEPATH += $(PERDAIX10PATH)/flightsoftware/libraries/sensors/ \
                $(PERDAIX10PATH)/flightsoftware/libraries/global/

LIBS += -L$(PERDAIX10PATH)/lib -lperdaixsensors -lperdaixglobal

SOURCES = main.cpp
HEADERS = main.h

QMAKE_LFLAGS -= -Wl,--as-needed

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
