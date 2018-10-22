/* > main.cpp
 * 1.00 arb Thu May 27 09:09:06 BST 2010
 */

static const char SCCSid[] = "@(#)main.cpp      1.00 (C) 2010 arb XQCT";

#include <qsplashscreen.h>
#include "satqt/qapp.h"
#include "satlib/dundee.h"
#include "xqct.h"


int
main(int argc, char **argv)
{
	// Global initialisations
	set_program_name(argv[0], NULL);

	QApp app(argc, argv);

	// If not specified otherwise logs and help files are in application dir
	if (getenv("HLOGDIR")==NULL)
	{
		static char hlogenv[FILENAME_MAX];
		sprintf(hlogenv, "HLOGDIR=%s", (const char*)qApp->applicationDirPath());
		putenv(hlogenv);
	}
	if (getenv("HHELPDIR")==NULL)
	{
		static char hhelpenv[FILENAME_MAX];
		sprintf(hhelpenv, "HHELPDIR=%s", (const char*)qApp->applicationDirPath());
		putenv(hhelpenv);
	}

	QSplashScreen splash(QPixmap::fromMimeSource("splash.png"));
	splash.message("Loading tide data", Qt::AlignHCenter, QColor("black"));
	splash.show();

	DisplayWindow *mainwin = new DisplayWindow();
	app.setMainWidget(mainwin);
	mainwin->setCaption( "XQCT - QuickChart Display" );
	mainwin->show();
	splash.finish(mainwin); // waits for mainwin to show

	return app.exec();
}
