TEMPLATE    = app
CONFIG      += qt warn_on debug thread unzip #exeprofile
DEFINES     += DEBUG
INCLUDEPATH += $(SWDEV)
LIBS        += -lsatqt -losmap -lsat -lutils -lgif -larb -ltcd
INTERFACES += configdialog.ui
HEADERS     = xqct.h   qctimage.h   tidedata.h   tidecalc.h   qctcollection.h
SOURCES     = xqct.cpp qctimage.cpp tidedata.cpp tidecalc.cpp qctcollection.cpp
SOURCES    += xqct_main.cpp
IMAGES      = splash.png
TARGET      = xqct

unzip {
	DEFINES += USE_UNZIP
}
