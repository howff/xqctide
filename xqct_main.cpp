/* > main.cpp
 * 1.00 arb Thu May 27 09:09:06 BST 2010
 */

static const char SCCSid[] = "@(#)main.cpp      1.00 (C) 2010 arb XQCT";

#include <QSplashScreen>
#include <QApplication>
#include <QPixmap>
//#include "satlib/dundee.h"
#include "xqct.h"


int
main(int argc, char **argv)
{
	// Global initialisations
	//set_program_name(argv[0], NULL);

	QApplication app(argc, argv);

	// If not specified otherwise logs and help files are in application dir
	if (getenv("HLOGDIR")==NULL)
	{
		static char hlogenv[FILENAME_MAX];
		sprintf(hlogenv, "HLOGDIR=%s", app.applicationDirPath().toLocal8Bit().constData());
		putenv(hlogenv);
	}
	if (getenv("HHELPDIR")==NULL)
	{
		static char hhelpenv[FILENAME_MAX];
		sprintf(hhelpenv, "HHELPDIR=%s", app.applicationDirPath().toLocal8Bit().constData());
		putenv(hhelpenv);
	}

	QSplashScreen splash(QPixmap(":/splash.png"));
	splash.message("Loading tide data", Qt::AlignHCenter, QColor("black"));
	splash.show();

	DisplayWindow *mainwin = new DisplayWindow();
	mainwin->setWindowTitle( "XQCT - QuickChart Display" );
	mainwin->show();
	splash.finish(mainwin); // waits for mainwin to show

	return app.exec();
}
