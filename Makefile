xqct: xqct.pro xqct.cpp xqct_main.cpp xqct.h qctimage.h qctimage.cpp tidedata.cpp tidedata.h tidecalc.h tidecalc.cpp qctcollection.h qctcollection.cpp
	qmake3 -o Makefile.${SWDEVARCH} xqct.pro
	make -f Makefile.${SWDEVARCH}

zip:
	zip xqct.zip *.pro *.mak *.cpp *.h *.C1 *.T1 Makefile* -x "*~" -x "moc_*" -x "*.o"

clean:
	rm -f *.o core xqct
	make -f Makefile.${SWDEVARCH} clean
	
