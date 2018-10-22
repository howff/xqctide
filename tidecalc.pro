TEMPLATE    = app
CONFIG      += qt warn_on debug thread
DEFINES     += DEBUG MAIN
INCLUDEPATH += $(SWDEV)
LIBS        += -lsatqt -lsat -lutils -lgif -larb -ltcd
HEADERS     = tidecalc.h
SOURCES     = tidecalc.cpp
TARGET      = tidecalc
