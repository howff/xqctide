TEMPLATE    = app
CONFIG      += qt warn_on debug thread
DEFINES     += DEBUG MAIN
INCLUDEPATH += $(SWDEV)
LIBS        += -lsatqt -lsat -lutils -lgif -larb
HEADERS     = tidedata.h
SOURCES     = tidedata.cpp
TARGET      = td
