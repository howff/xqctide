/* > xqct.cpp
 * 1.01 arb Fri Jul 30 19:05:09 BST 2010 - Finished nice version.
 * 1.00 arb Sat Jul 10 19:47:16 BST 2010 - Finished most basic version.
 * 0.01 arb Mon May 17 10:20:36 BST 2010
 */

static const char SCCSid[] = "@(#)xqct.cpp      1.01 (C) 2010 arb QuickChart display";


/*
 * Displays a georeferenced QuickChart with tidal information superimposed.
 */


/*
 * Configuration:
 * Define RESTRICT_TIDAL_DIAMONDS_TO_SELECTED_CHART if you want to
 *  only load tidal diamonds which are defined to be on the loaded chart.
 *  Sometimes larger scale charts have diamonds for the same area which
 *  it can be useful to load and compare but normally they are worse so
 *  you may want to define this to get rid of them.
 * Define RESTRICT_TIDAL_LEVELS_TO_SELECTED_CHART similarly.
 * Define ARROW_SCALE to make them visible.
 * Define DEFAULT_CHART as first chart to load
 * Define DEFAULT_TIDE_DATA_DIR as directory containing .C1 and .T1 files
 * Define DEFAULT_HARMONICS_FILE as 
 */
#define DEFAULT_TL_MUST_BE_ON_CHART    false // could be on other charts
#define DEFAULT_TL_MUST_BE_UNIQUE      true  // XXX should be true after debugged
#define DEFAULT_TS_MUST_BE_ON_CHART    false // could be on other charts
#define DEFAULT_TS_MUST_BE_UNIQUE      true  // XXX should be true after debugged
#define DEFAULT_TS_MUST_HAVE_KNOWN_REF true  // pointless if TS ref station is unknown
#define ARROW_SCALE 40
#define DEFAULT_CHART          "BA1481_1.qct"
//#define DEFAULT_CHART_DIR      "/users/local/arb/h/memorymap/v-g"
//#define DEFAULT_CHART_DIR      "/mnt/hgfs/VM/tide/maptech"
#define DEFAULT_CHART_DIR       qApp->applicationDirPath()+DIRSEPSTR+"qct"
#define DEFAULT_TIDE_DATA_DIR   qApp->applicationDirPath()
#define DEFAULT_TIDEC1_ZIP     "tidec1.zip"
#define DEFAULT_TIDET1_ZIP     "tidet1.zip"
#define DEFAULT_TIDE_ZIP_PASSWORD QString::null
//#define DEFAULT_HARMONICS_FILE "harmonics-dwf-20091227-nonfree.tcd"
#define DEFAULT_HARMONICS_FILE "tcd/world.tcd"
#define DEFAULT_ZOOM_OUT_LEVEL 1 // full size
#define PRINTER_MARGIN_CM      1 // 1 cm margins
#define TIDE_MAX_LINE_LEN    512 // typically 450 bytes max

/*
 * Bugs:
 * Still some jumps/sticky bits in bearing when new HW time is used,
 *  maybe caused by tables which don't wrap properly;
 *  need to think about wraparound or extrapolation rather than clipping.
 * WARNING asking for tide at offset 968871.120000 hrs probably caused by
 * the simple-minded search for high tide rather than calculating explicitly.
 * Speed up TCD using a QDict cache of n,lat,lon rather than a prev String.
 * Speed up moon phase - if jtime within synodic month of last jtime.
 * Default zoom level in prefs is fairly useless.
 * refAtHW is not used yet (it all breaks if value is false).
 */

/*
 * To do:
 * Date editing: remember last used date or today's date for next time,
 *  button for today, buttons for prev/next day/week
 *  Slider at bottom for day?
 * Printing:
 *   Print at full resolution
 * Map display:
 *   Load tiles as needed instead of all at once
 *   Scale down uses colour interpolation vertically as well as horiz
 *   Zoom in/out should reload just image data not whole QCT file
 *     (then wouldn't have to reload tide data either)
 *     Same code could be used by printing to get full resolution.
 * Nice to have:
 *   Show moon phase in status bar
 *   Overlays, eg.
 *     positions of wrecks,
 *     boat launch locations,
 *   Zoom should reposition based on mouse position not map center
 *   Collect all output (errors etc) into a log window
 *   Is a graph of tide height or current over time useful?
 */


#include <qaccel.h>
#include <qapplication.h>
#include <qcolordialog.h>
#include <qdragobject.h>
#include <qfile.h>
#include <qfiledialog.h>
#include <qgrid.h>
#include <qinputdialog.h>
#include <qkeycode.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpopupmenu.h>
#include <qpaintdevicemetrics.h> // for printing
#include <qprinter.h>            // for printing
#include <qregexp.h>
#include <qsettings.h>
#include <qslider.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qtoolbar.h>
#include <qtoolbutton.h>
#include <qvbox.h>
#include <qwhatsthis.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "satlib/dundee.h"
#include "satqt/qapp.h"        // for UNICODE_ definitions
#include "satqt/helpdialog.h"
#include "satqt/mrumenu.h"
#include "satqt/inputdialog.h"
#include "satqt/qzip.h"
#include "satqt/calendardialog.h"
#include "satqt/fileopen.xpm"
#include "satqt/filesave.xpm"
#include "satqt/fileprint.xpm"
#include "satqt/calendaricon3.xpm"
#include "osmap/qct.h"
#include "satqt/kconfig.h"            // for KAutoConfig
#include "satqt/kautoconfigdialog.h"  // for KAutoConfig
#include "configdialog.h"                   // ditto
#include "qctimage.h"
#include "qctcollection.h"
#include "tidedata.h"
#include "tidecalc.h"
#include "xqct.h"

#define UNUSED(x) ((x)=(x)) /* keep compiler quiet */

static const char * progname = "XQCT";
static const char *LOG_E = "error";
static const char *LOG_I = "xqct";


/* ----------------------------------------------------------------------------
 * For checking whether two tidal locations are close enough that they would
 * overlap or in fact are identical duplicates and so only one is necessary.
 * XXX should get pixel resolution in degrees from QCT and return closeEnough
 * if with so many pixels rather than so many degrees.
 */
static double
distanceBetween(double lat0, double lon0, double lat1, double lon1)
{
	// Should calculate Great Circle distance, but OK for close points
	// Don't bother to take the square root for speed.
	//debugf(1,"dist %f->%f and %f->%f = %f\n",lat0,lat1,lon0,lon1,((lat1-lat0)*(lat1-lat0) + (lon1-lon0)*(lon1-lon0)));
	return ((lat1-lat0)*(lat1-lat0) + (lon1-lon0)*(lon1-lon0));
}

static bool
closeEnough(double distSquared)
{
	// If absolute difference is less than 0.00045 then close (~50 meters of latitude)
	// return (distSquared < (0.00045 * 0.00045));
	// If absolute difference is less than 0.0009 then close (~100 meters of latitude)
	// return (distSquared < (0.0009 * 0.0009));
	// Some of the tidal stream locations are so far off that we need to return this to filter them:
	return (distSquared < (0.00155 * 0.00155));
}

static bool
closeEnough(double distSquared, double degreesPerPixel)
{
	double dist = sqrt(distSquared);
	double pixels = dist / degreesPerPixel;
	//debugf(1,"dist %f deg at %f deg per pix (%f pix per deg) is %f pixels\n",dist,degreesPerPixel,1.0/degreesPerPixel,pixels);
	return (pixels < 20.0);
}


/* ----------------------------------------------------------------------------
 * Main window
 */
DisplayWindow::DisplayWindow()
    : QMainWindow( 0, "xqct main window", WDestructiveClose)
{
	int id;

	//settings = new QSettings();
	settings = new KConfig("xqct"); // replaces QSettings(); reads file $HSYSDIR/xqctrc
	const char *hsysdir = getenv(SYSCONFIGENV);
	if (hsysdir)
		settings->insertSearchPath(QSettings::Unix, hsysdir);
	settings->insertSearchPath(QSettings::Windows, "/DSS");
	settings->beginGroup("xqct");
	int wx = settings->readNumEntry("x", -1);
	int wy = settings->readNumEntry("y", -1);
	if (wx != -1 && wy != -1)
		move(wx, wy);
	wx = settings->readNumEntry("width", -1);
	wy = settings->readNumEntry("height", -1);
	if (wx != -1 && wy != -1)
		resize(wx, wy);
	loadSettings();

	// Create "File" menu
	QPopupMenu * fileMenu = new QPopupMenu( this );
	menuBar()->insertItem( "&File", fileMenu );
	mruMenu = new MRUMenu(this, settings, NULL);
	// mruMenu->setShortcuts(true); // useless as only activated after menu shown!
	connect(mruMenu, SIGNAL(activatedText(const QString &, const QString &)), this, SLOT(loadNewMapFile(const QString &, const QString &)));

	fileMenu->insertItem( "&Open Map...", this, SLOT( loadMap() ), CTRL+Key_O );
	fileMenu->insertItem( "&Open Recent Map",  mruMenu );
	fileMenu->insertItem( "&Save Map As...", this, SLOT( saveScreen() ), CTRL+Key_S );
	//fileMenu->insertItem( "&Save Map As...", this, SLOT( saveMap() ), CTRL+Key_S );
	//fileMenu->insertItem( "Print Whole Map...", this, SLOT( printMap() ) ); // usually too big to be useful
	fileMenu->insertItem( "&Print Map...", this, SLOT( printScreen() ), CTRL+Key_P );
	fileMenu->insertSeparator();
	fileMenu->insertItem( "&Quit", this, SLOT( quit() ), CTRL+Key_Q );


	// Create "Edit" menu
	editMenu = new QPopupMenu( this );
	menuBar()->insertItem( "&Edit", editMenu );
	id = editMenu->insertItem( "&Date...", this, SLOT( editDate() ), CTRL+Key_D);
	id = editMenu->insertItem( "&Preferences", this, SLOT( editPrefs() ));
	// XXX whatsThis


	// Create "View" menu
	viewMenu = new QPopupMenu( this );
	menuBar()->insertItem( "&View", viewMenu );

	tidalLevelMenu = new QPopupMenu(viewMenu);
	connect(tidalLevelMenu, SIGNAL(aboutToShow()), this, SLOT(showTidalLevelMenu()));
	id = viewMenu->insertItem("Tidal &Level Locations", tidalLevelMenu);

	tidalStreamMenu = new QPopupMenu(viewMenu);
	connect(tidalStreamMenu, SIGNAL(aboutToShow()), this, SLOT(showTidalStreamMenu()));
	id = viewMenu->insertItem("Tidal &Diamond", tidalStreamMenu);

	// Create "Zoom" menu
	QPopupMenu *zoomMenu = new QPopupMenu(viewMenu);
	id = zoomMenu->insertItem("&Full size",    this, SLOT(changeZoom(int)), 0, 1);
	id = zoomMenu->insertItem("&Half size",    this, SLOT(changeZoom(int)), 0, 2);
	id = zoomMenu->insertItem("&Quarter size", this, SLOT(changeZoom(int)), 0, 4);
	id = zoomMenu->insertItem("&Eighth size",  this, SLOT(changeZoom(int)), 0, 8);
	id = viewMenu->insertItem("&Zoom", zoomMenu);

	// Create "Help" menu
	QPopupMenu * helpMenu = new QPopupMenu( this );
	menuBar()->insertSeparator();
	menuBar()->insertItem( "&Help", helpMenu );

	helpMenu->insertItem( "&About", this, SLOT(about()), Key_F1 );
	helpMenu->insertItem( "&Manual", this, SLOT(helpmanual()) );
	helpMenu->insertSeparator();
	helpMenu->insertItem( "What's &This", this, SLOT(whatsThis()), SHIFT+Key_F1 );

	QToolBar * fileTools = new QToolBar( this, "chart toolbar" );
	fileTools->setLabel( "Chart Toolbar" );
	new QToolButton( QPixmap(fileopen), "Open Map", QString::null,
		this, SLOT(loadMap()), fileTools, "open map" );
	new QToolButton( QPixmap(filesave), "Save Map", QString::null,
		this, SLOT(saveScreen()), fileTools, "save map" );
	new QToolButton( QPixmap(fileprint), "Print Map", QString::null,
		this, SLOT(printScreen()), fileTools, "print map" );
	new QToolButton( QPixmap(calendaricon3), "Change Date", QString::null,
		this, SLOT(editDate()), fileTools, "change date" );

	// Main window contains a vertical box which groups:
	// the map image, a slider, a label (status bar is at bottom)
	QVBox *main = new QVBox(this);

	qctimage = new QCTImage(main);
	// for information:
	//qctimage->getQct()->printMetadata(stdout);
	connect(qctimage, SIGNAL(location(double,double,long)), this, SLOT(mouse_moved(double,double)));
	connect(qctimage, SIGNAL(mouseWheel(Qt::Orientation, int)), this, SLOT(mouse_wheel(Qt::Orientation, int)));
	connect(qctimage, SIGNAL(contextMenu(double,double)), this, SLOT(context_menu(double,double)));

	slider = new QSlider(0, TIDECALC_PERDAY*3-1, 60/TIDECALC_INTERVAL_MINS, 0, QSlider::Horizontal, main, "slider");
	slider->setFocus();
	slider->setTickInterval(6);
	slider->setTickmarks(QSlider::Below);
	connect(slider, SIGNAL(valueChanged(int)), this, SLOT(slider_moved(int)));

	// Coordinate display
	QHBox *infobox = new QHBox(main);
	dateLabel = new QLabel(infobox, "dat"); dateLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	latlonDegLabel = new QLabel(infobox, "deg"); latlonDegLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	latlonDMSLabel = new QLabel(infobox, "dms"); latlonDMSLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	bngLabel = new QLabel(infobox, "bng"); bngLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	// Choose the coming Saturday
	QDate today = QDate::currentDate();
	if (today.dayOfWeek() < 6)
	{
		// Monday..Sunday is 1..7 so if before weekend then choose Saturday
		today = today.addDays(6 - today.dayOfWeek());
	}
	// Can't call setDate(today.year(), today.month(), today.day(), 0, 0)
	// because that moves the slider which does the tide calcs and we're not
	// ready for that yet so just set the time manually
	slider_jtime = date_to_jtime(today.year(), today.month(), today.day(), 0, 0);
	slider_offset = 0.0;

	// Start collecting maps (this will continue in a background thread)
	// ie. in the constructor it calls mapCollectionPtr->collectMaps();
	mapCollectionPtr = new QCTCollection(DEFAULT_CHART_DIR);

	// Internal data
	tidalStreamList.clear();
	tidalStreamList.setAutoDelete(true);
	tidalLevelList.clear();
	tidalLevelList.setAutoDelete(true);
	tideCalcPtr = new TideCalc();
	moonCalcPtr = new MoonCalc();

	// Load the TCD file tide station database
	tideCalcPtr->loadTideDatabase(DEFAULT_HARMONICS_FILE);

	// Load the first map (last one used if possible)
	if (!mruMenu->mostRecentEntry().isEmpty())
	{
		loadNewMapFile(mruMenu->mostRecentEntry(), mruMenu->mostRecentMetadata());
	}
	else
		loadNewMapFile(DEFAULT_CHART);

	setCentralWidget( main );

	statusBar()->message("Ready", 1000);
}


DisplayWindow::~DisplayWindow()
{
	saveMapMetadata();
	saveSettings();
	delete settings;

	delete tideCalcPtr;
	delete moonCalcPtr;
	delete mapCollectionPtr;
}


/* ----------------------------------------------------------------------------
 */
void DisplayWindow::loadSettings()
{
#if 0
	QStringList list;
	QStringList::Iterator iter;
	settings->endGroup(); // grrr kautoconfig leaves it in a random group
	list = settings->entryList("/xqct");
	for (iter = list.begin(); iter != list.end(); ++iter )
	{
		QString val = settings->readEntry(QString("/xqct/") + *iter, "<null>");
		printf("entryList: %s = %s\n", (const char*) *iter, (const char*)val);
	}
	list = settings->subkeyList("/xqct");
	for (iter = list.begin(); iter != list.end(); ++iter )
	{
		printf("subkeyList: %s\n", (const char*) *iter);
		QStringList list2;
		QStringList::Iterator iter2;
		QString subkey("/xqct/");
		subkey += *iter;
		printf("  looking at %s\n", (const char*) subkey);
		list2 = settings->entryList(subkey);
		for (iter2 = list2.begin(); iter2 != list2.end(); ++iter2 )
		{
			printf("  entryList: %s\n", (const char*) *iter2);
		}
	}
#endif
	cfg_TLmustBeOnChart    = settings->readBoolEntry("TLmustBeOnChart", DEFAULT_TL_MUST_BE_ON_CHART);
	cfg_TLmustBeUnique     = settings->readBoolEntry("TLmustBeUnique",  DEFAULT_TL_MUST_BE_UNIQUE);
	cfg_TSmustBeOnChart    = settings->readBoolEntry("TSmustBeOnChart", DEFAULT_TS_MUST_BE_ON_CHART);
	cfg_TSmustBeUnique     = settings->readBoolEntry("TSmustBeUnique",  DEFAULT_TS_MUST_BE_UNIQUE);
	cfg_TSmustHaveKnownRef = settings->readBoolEntry("TSmustHaveKnownRef", DEFAULT_TS_MUST_HAVE_KNOWN_REF);
	// map 0,1,2,3 to 1,2,4,8
	cfg_zoomOutLevel       = 1 << settings->readNumEntry("zoomOutLevel", DEFAULT_ZOOM_OUT_LEVEL);
}


/* ----------------------------------------------------------------------------
 */
void DisplayWindow::saveSettings()
{
	// map 1,2,4,8 to 0,1,2,3
	int level = cfg_zoomOutLevel==8?3:(cfg_zoomOutLevel==4?2:(cfg_zoomOutLevel==2?1:0));
	//printf("saveSettings zoom %d save %d\n", cfg_zoomOutLevel, level);
	//printf("saveSettings TSmustBeUnique=%s\n", cfg_TSmustBeUnique?"true":"false");
	settings->writeEntry("x", x());
	settings->writeEntry("y", y());
	settings->writeEntry("width", width());
	settings->writeEntry("height", height());
	// XXX the bools get written as integers (0 or 1) for some reason
	// maybe because we're using KConfig instead of QSettings?
	// It still works though.
	// KAutoConfig likes to actually delete entries if the value is
	// the same as the default (which is kind of nice if you later
	// change the default) but here we just write every setting.
	settings->writeEntry("TLmustBeOnChart",    cfg_TLmustBeOnChart);
	settings->writeEntry("TLmustBeUnique",     cfg_TLmustBeUnique);
	settings->writeEntry("TSmustBeOnChart",    cfg_TSmustBeOnChart);
	settings->writeEntry("TSmustBeUnique",     cfg_TSmustBeUnique);
	settings->writeEntry("TSmustHaveKnownRef", cfg_TSmustHaveKnownRef);
	settings->writeEntry("zoomOutLevel",       level);
}


/* ----------------------------------------------------------------------------
 */
void DisplayWindow::slider_moved(int pos)
{
	slider_offset = pos * TIDECALC_INTERVAL_MINS;
	debugf(1, "slider_moved pos %d is mins %f\n", pos, slider_offset);

	// Update the current time in the status bar
	dateLabel->setText(QString(jctime(slider_jtime + slider_offset)));

	QApplication::setOverrideCursor(waitCursor);
	plotTidalStreams();
	plotTidalLevels();
	QApplication::restoreOverrideCursor();
}


/* ----------------------------------------------------------------------------
 * Mouse has moved so update the position text
 */
QString
coord_str_deg(double lat, double lon)
{
	QString msg;
	msg.sprintf("%8.4f %c, %8.4f %c", fabs(lat), lat>0?'N':'S', fabs(lon), lon>0?'E':'W');
	return msg;
}


QString
coord_str_dms(double lat, double lon)
{
	QString msg;
	int neglat = (lat < 0);
	int neglon = (lon < 0);
	lat = fabs(lat);
	lon = fabs(lon);
	double latmin = lat - (int)lat;
	double lonmin = lon - (int)lon;
	msg.sprintf("%d* %.2f' %c, %d* %.2f' %c", (int)lat, latmin*60.0, neglat?'S':'N', (int)lon, lonmin*60.0, neglon?'W':'E');
	msg.replace("*", QString(QChar(UNICODE_DEGREE)));
	return msg;
}


QString
coord_str_bng(double lat, double lon)
{
	QString msg;
	return msg;
}


void
DisplayWindow::mouse_moved(double lat, double lon)
{
	latlonDegLabel->setText(coord_str_deg(lat, lon));
	latlonDMSLabel->setText(coord_str_dms(lat, lon));
	bngLabel->setText(coord_str_bng(lat, lon));
}


/* ----------------------------------------------------------------------------
 * Mouse wheel has been scrolled, zoom in or out
 */
void
DisplayWindow::mouse_wheel(Qt::Orientation orient, int delta)
{
	if (orient == Qt::Horizontal)
		return;
	bool positive = delta > 0;
	int newzoom = cfg_zoomOutLevel;
	if (!positive && newzoom < 8)
	{
		newzoom <<= 1;
	}
	else if (positive && newzoom > 1)
	{
		newzoom >>= 1;
	}
	debugf(1,"mouse_wheel delta %d new level %d\n",delta,newzoom);
	if (newzoom != cfg_zoomOutLevel)
		changeZoom(newzoom);
}


/* ----------------------------------------------------------------------------
 * Context menu requested at given latitude/longitude.
 * Show list of maps which cover that location
 * and look through all tidal levels and tidal streams to see if any are close
 * and add a menu entry to get more details.
 */
void
DisplayWindow::context_menu(double latN, double lonE)
{
	int id, nn;

	debugf(1,"ContextMenu %f %f\n", latN, lonE);

	// Create a menu
	QPopupMenu *contextMenu = new QPopupMenu(this);

	// Add a submenu listing available maps
	mapContextMenu = new QPopupMenu(contextMenu);
	id = contextMenu->insertItem("Maps at cursor", mapContextMenu);

	QStringList maps = mapCollectionPtr->mapsListAtLatLon(latN, lonE);
	QStringList::Iterator mapiter;
	nn = 0;
	for (mapiter = maps.begin(); mapiter != maps.end(); ++mapiter)
	{
		debugf(1,"Map at cursor: %s\n", (const char *)*mapiter);
		id = mapContextMenu->insertItem(*mapiter, this, SLOT(context_menu_map(int)), 0, nn++);
	}
	if (nn == 0)
	{
		id = mapContextMenu->insertItem("None");
		mapContextMenu->setItemEnabled(id, false);
	}

	// Add an entry for tidal levels
	nn = 0;
	for ( TidalLevel *tlpiter = tidalLevelList.first(); tlpiter; tlpiter = tidalLevelList.next() )
	{
		if (closeEnough(distanceBetween(latN, lonE, tlpiter->getLat(), tlpiter->getLon()), qctimage->getQct()->getDegreesPerPixel()))
		{
			//debugf(2, "  %s at %f %f\n", (const char*)tlpiter->getName(), tlpiter->getLat(), tlpiter->getLon());
			id = contextMenu->insertItem(tlpiter->getName(), this, SLOT(context_menu_level(int)), 0, nn);
		}
		nn++;
	}

	// Add an entry for tidal streams
	nn = 0;
	for ( TidalStream *tspiter = tidalStreamList.first(); tspiter; tspiter = tidalStreamList.next() )
	{
		if (closeEnough(distanceBetween(latN, lonE, tspiter->getLat(), tspiter->getLon()), qctimage->getQct()->getDegreesPerPixel()))
		{
			//debugf(2, "  %s at %f %f ref %s\n", (const char*)tspiter->getName(), tspiter->getLat(), tspiter->getLon(), (const char*)tspiter->getRef());
			id = contextMenu->insertItem(tspiter->getName() + " via "+tspiter->getRef(), this, SLOT(context_menu_stream(int)), 0, nn);
		}
		nn++;
	}

	contextMenu->exec(QCursor::pos());
	delete contextMenu;
}


void
DisplayWindow::context_menu_map(int id)
{
	debugf(1, "Request map number %d = %s\n", id, (const char*) mapContextMenu->text(id));
	// Lookup map name by extracting it from the menu item chosen
	// then lookup the map filename by querying the map collection
	// then load it (will save the current map position, load new one
	// and jump to same position)
	QString fn = mapCollectionPtr->getFilenameForMap(mapContextMenu->text(id));
	debugf(1, "  so load filename %s\n", (const char*)fn);
	loadNewMapFile(mapCollectionPtr->getFilenameForMap(mapContextMenu->text(id)));
}


void
DisplayWindow::context_menu_level(int id)
{
	debugf(1, "Context menu Level %d\n", id);
	int nn = 0;
	for ( TidalLevel *tlpiter = tidalLevelList.first(); tlpiter; tlpiter = tidalLevelList.next() )
	{
		if (nn == id)
		{
			debugf(1, "  is %s\n", (const char*)tlpiter->getName());
			QMessageBox::information(this, "Tidal Level",
				QString("<p>The tidal level %1 has:<ul>"
				"<li>Mean High Water (Spring) %2"
				"<li>Mean Low Water (Spring) %3"
				"<li>Mean High Water (Neap) %4"
				"<li>Mean Low Water (Neap) %5"
				"<li>Current Level %6</ul>")
				.arg(tlpiter->getName())
				.arg(tlpiter->getMHWS())
				.arg(tlpiter->getMLWS())
				.arg(tlpiter->getMHWN())
				.arg(tlpiter->getMLWN())
				.arg(tlpiter->getCurrentLevel()));
			break;
		}
		nn++;
	}
}


void
DisplayWindow::context_menu_stream(int id)
{
	debugf(1, "Context menu Stream %d\n", id);
	int nn = 0;
	for ( TidalStream *tspiter = tidalStreamList.first(); tspiter; tspiter = tidalStreamList.next() )
	{
		if (nn == id)
		{
			debugf(1, "  is %s\n", (const char*)tspiter->getName());
			float bearing = tspiter->getCurrentBearing();
			QMessageBox::information(this, "Tidal Stream",
				QString("<p>The tidal stream %1 is:<ul>"
				"<li>referenced to the tidal station %2"
				"<li>current bearing %3 degrees clockwise from North"
				"<li>current rate %4 knots</ul>")
				.arg(tspiter->getName())
				.arg(tspiter->getRef())
				.arg(bearing < 0 ? bearing+360.0 : bearing)
				.arg(tspiter->getCurrentRate()));
			break;
		}
		nn++;
	}
}


/* ----------------------------------------------------------------------------
 * Slot called by View menu (menu id is actually the desired zoom level 1,2,4,8)
 * or from mouse_wheel
 */
void
DisplayWindow::changeZoom(int id)
{
	debugf(1,"Changezoom from %d to %d\n", cfg_zoomOutLevel, id);
	cfg_zoomOutLevel = id;
	// XXX check map already loaded first??
	double lat, lon;
	qctimage->latLonOfCenter(&lat, &lon);
	debugf(1,"map center before %f %f\n",lat,lon);
	loadNewMapFile(mapFilename);
	qctimage->scrollToLatLon(lat, lon);
	debugf(1,"map center after  %f %f\n",lat,lon);
}


/* ----------------------------------------------------------------------------
 * Menu listing all Tidal Levels (where tide heights can be calculated)
 */
void
DisplayWindow::showTidalLevelMenu()
{
	int id, nn = 0;
	tidalLevelMenu->clear();
	for ( TidalLevel *tlpiter = tidalLevelList.first(); tlpiter; tlpiter = tidalLevelList.next() )
	{
		QString label(tlpiter->getName());
		id = tidalLevelMenu->insertItem(label, this, SLOT(tidalLevelMenuSelected(int)), 0, nn++);
	}
	if (!nn)
		tidalLevelMenu->setItemEnabled(tidalLevelMenu->insertItem("No Tidal Levels loaded or none on this chart"), false);
}

void
DisplayWindow::tidalLevelMenuSelected(int id)
{
	if (id < 0 || id > tidalLevelList.count()-1)
		return;
	qctimage->scrollToLatLon(tidalLevelList.at(id)->getLat(),
		tidalLevelList.at(id)->getLon());
}


/* ----------------------------------------------------------------------------
 * Menu listing all Tidal Streams (Tidal Diamonds)
 */
void
DisplayWindow::showTidalStreamMenu()
{
	int id, nn = 0;
	tidalStreamMenu->clear();
	for ( TidalStream *tspiter = tidalStreamList.first(); tspiter; tspiter = tidalStreamList.next() )
	{
		QString label(tspiter->getName());
		label += " on " + tspiter->getChart();
		label += " via " + tspiter->getRef();
		id = tidalStreamMenu->insertItem(label, this, SLOT(tidalStreamMenuSelected(int)), 0, nn++);
	}
	if (!nn)
		tidalStreamMenu->setItemEnabled(tidalStreamMenu->insertItem("No Tidal Streams loaded or none on this chart"), false);
}

void
DisplayWindow::tidalStreamMenuSelected(int id)
{
	if (id < 0 || id > tidalStreamList.count()-1)
		return;
	qctimage->scrollToLatLon(tidalStreamList.at(id)->getLat(),
		tidalStreamList.at(id)->getLon());
}


/* ----------------------------------------------------------------------------
 * loadMap - called from menu, prompts for a *.qct file to load
 * loadNewMapFile - called from loadMap and from mruMenu loads map then tide
 * loadMapFile - called from load(New)Map or changeZoom does the loading
 * the distinction is that changing the zoom doesn't require tide reload
 * and doesn't want previous zoom/center to be saved in the MRU menu
 */
void
DisplayWindow::saveMapMetadata()
{
	double lat, lon;

	// Store the current map, position and zoom in the MRU metadata
	// This assumes nothing has fiddled with the most recent entry
	// since loadMapFile last used mruMenu->add.
	if (qctimage->latLonOfCenter(&lat, &lon))
	{
		QString metadata;
		metadata.sprintf("%d;%f;%f", cfg_zoomOutLevel, lat, lon);
		mruMenu->setMostRecentMetadata(metadata);
	}
}

void
DisplayWindow::loadMap()
{
	QString qctdir(DEFAULT_CHART_DIR);
	QString qctname( QFileDialog::getOpenFileName(qctdir, "Maps and charts (*.qct)", this) );
    if (qctname.isEmpty())
        return;

	loadNewMapFile(qctname);
}


void
DisplayWindow::loadNewMapFile(const QString &qctname, const QString &qctMetadata)
{
	// Store the current map, position and zoom in the MRU metadata
	// This assumes nothing has fiddled with the most recent entry
	// since loadMapFile last used mruMenu->add.
	saveMapMetadata();

	// Load the new map and reposition/zoom it
	loadMapFile(qctname, qctMetadata);

	// Reload the tide info to get new arrows
	loadTidalData();

	// Plot the overlays
	QApplication::setOverrideCursor(waitCursor);
	plotTidalStreams();
	plotTidalLevels();
	QApplication::restoreOverrideCursor();

	// Update the current time in the status bar
	dateLabel->setText(QString(jctime(slider_jtime + slider_offset)));
}


void
DisplayWindow::loadMapFile(const QString &qctname, const QString &qctMetadata)
{
	debugf(1, "loadMapFile %s\n",(const char*)qctname);

	// Decode the metadata (zoom and center)
	double lat = 0, lon = 0;
	if (!qctMetadata.isEmpty())
	{
		QStringList parts = QStringList::split(';', qctMetadata);
		if (!parts[0].isEmpty())
			cfg_zoomOutLevel = parts[0].toInt();
		if (!parts[1].isEmpty())
			lat = parts[1].toDouble();
		if (!parts[1].isEmpty())
			lon = parts[2].toDouble();
	}

	// Free the previous image
	qctimage->unload();

	// Load the new image at the configured zoom level
	// Failure refreshes the screen to remove old one
	if (!qctimage->load(qctname, cfg_zoomOutLevel))
	{
		qctimage->updateContents();
		return;
	}

	// Save as most recent entry
	// If metadata given then preserve it for this file too
	mruMenu->add(qctname);
	mruMenu->setMostRecentMetadata(qctMetadata);

	mapFilename = qctname;

	// Set the window title to include the map name
	QString title("XQCT - ");
	title += qctimage->getQct()->getName();
	debugf(1, "setCaption '%s'\n", (const char*)title);
	setCaption(title);

	// Metadata contains location of center to view
	if (lat && lon)
	{
		qctimage->scrollToLatLon(lat, lon);
	}

	// Refresh the whole window
	qctimage->updateContents();
}


/* ----------------------------------------------------------------------------
 * loadTidalData reads all *.C1 and *.T1 files containing tidal information,
 * sifts out the duplicates according to the cfg_ preferences
 * and appends to the lists tidalLevelList and tidalStreamList.
 */

bool
DisplayWindow::loadTidalLevelFile(const QString &zipname, const QString &zippassword, const QString &filename)
{
#ifdef USE_UNZIP
	ZipFile file(zipname);
	file.setName(filename, zippassword);
	if (!file.open(IO_ReadOnly))
		return false;
#else
	QFile file(filename);
	debugf(1, "load %s\n", (const char*)file.name());
	if (!file.open(IO_ReadOnly))
		return false;
#endif
	QString line;
	while (file.readLine(line, TIDE_MAX_LINE_LEN) > 0)
	{
		//debugf(1, "TL::%s", (const char*)line);
		TidalLevel *tlp, *tlpiter;
		tlp = new TidalLevel(line);
		if (!tlp->isOk())
		{
			delete tlp;
			continue;
		}
		// Ignore tide levels not on this chart
		if (cfg_TLmustBeOnChart && (qctimage->getQct()->getIdentifier() != tlp->getChart()))
		{
			//debugf(1, "TL: %s != %s\n", (const char*)qctimage->getQct()->getIdentifier(), (const char*)tlp->getChart());
			delete tlp;
			continue;
		}
		// Ignore tide levels with coords outside boundary of this chart
		if (!qctimage->getQct()->coordInsideMap(tlp->getLat(), tlp->getLon()))
		{
			//debugf(1, "TL: %s at %f,%f is not on this chart\n", (const char*)tlp->getName(), tlp->getLat(), tlp->getLon());
			delete tlp;
			continue;
		}
		// Check for duplicates, just ignore the new one
		// assuming they actually are identical and first is ok
		if (cfg_TLmustBeUnique)
			for ( tlpiter = tidalLevelList.first(); tlpiter; tlpiter = tidalLevelList.next() )
		{
			if (closeEnough(distanceBetween(tlpiter->getLat(), tlpiter->getLon(), tlp->getLat(), tlp->getLon()), qctimage->getQct()->getDegreesPerPixel()))
			{
				debugf(2,"  IGNORE %s - same location as %s\n", (const char*)tlp->getName(), (const char*)tlpiter->getName());
				delete tlp;
				tlp = 0;
				break;
			}
		}
		if (tlp == 0)
			continue;
		debugf(1, "Tide Level: Spring Tide range %.1f to %.1f at %s\n", tlp->getMLWS(), tlp->getMHWS(), (const char*)tlp->getName());
		debugf(1, "Tide Level: Neap Tide   range %.1f to %.1f at %s\n", tlp->getMLWN(), tlp->getMHWN(), (const char*)tlp->getName());
		tidalLevelList.append(tlp);
	}
	file.close();
	return true;
}


bool
DisplayWindow::loadTidalStreamFile(const QString &zipname, const QString &zippassword, const QString &filename)
{
#ifdef USE_UNZIP
	ZipFile file(zipname);
	file.setName(filename, zippassword);
	if (!file.open(IO_ReadOnly))
		return false;
#else
	QFile file(filename);
	debugf(1, "load %s\n", (const char*)file.name());
	if (!file.open(IO_ReadOnly))
		return false;
#endif
	QString line;
	while (file.readLine(line, TIDE_MAX_LINE_LEN) > 0)
	{
		TidalStream *tsp, *tspiter;
		tsp = new TidalStream(line);
		if (!tsp->isOk())
		{
			delete tsp;
			continue;
		}
		// Ignore tide streams not on this chart
		// Can be useful to see all diamonds from other charts though
		// (assuming they are within the chart boundary checked below)
		if (cfg_TSmustBeOnChart && (qctimage->getQct()->getIdentifier() != tsp->getChart()))
		{
			//printf("TS: %s != %s\n", (const char*)qctimage->getQct()->getIdentifier(), (const char*)ts.getChart());
			delete tsp;
			continue;
		}
		// Ignore tide levels off this chart
		if (!qctimage->getQct()->coordInsideMap(tsp->getLat(), tsp->getLon()))
		{
			//printf("TS: %s at %f,%f is not on this chart\n", (const char*)ts.getName(), ts.getLat(), ts.getLon());
			delete tsp;
			continue;
		}
		//debugf(1, "READ %s\n", (const char*)line);
		debugf(1, "Tidal Stream %s referenced to %s at %sW (%f, %f)\n", (const char*)tsp->getName(), (const char*)tsp->getRef(), tsp->refAtHW()? "H":"L", tsp->getLat(), tsp->getLon());
		// Ignore tide streams if ref station is not in tide database
		if (cfg_TSmustHaveKnownRef)
		{
			double lat, lon;
			if (!tideCalcPtr->getStationLocation(tsp->getRef(), &lat, &lon))
			{
				log_message(LOG_I, stderr, "TidalStream ignored because %s is not a recognised location", (const char*)tsp->getRef());
				debugf(2, "  IGNORED %s - ref not in tide db %s\n", (const char*)tsp->getName(), (const char*)tsp->getRef());
				delete tsp;
				continue;
			}
		}
		// See if there's already one at the location
		if (cfg_TSmustBeUnique)
			for ( tspiter = tidalStreamList.first(); tspiter; tspiter = tidalStreamList.next() )
		{
			if (closeEnough(distanceBetween(tspiter->getLat(), tspiter->getLon(), tsp->getLat(), tsp->getLon()), qctimage->getQct()->getDegreesPerPixel()))
			{
				// If both have the same reference station and location then they are
				// hopefully identical (or we can't tell which is best) so ignore new one
				if (tspiter->getRef() == tsp->getRef())
				{
					debugf(2, "  IGNORED - same location AND ref station, so identical\n");
					delete tsp;
					tsp = 0;
					break;
				}
				// Find out which one has the closest reference station
				double reflat0, reflon0, reflat1, reflon1;
				double refdist1, refdist2;
				debugf(2, "  compare %s\n", (const char*)tsp->getRef());
				tideCalcPtr->getStationLocation(tsp->getRef(), &reflat0, &reflon0);
				debugf(2, "  to      %s\n", (const char*)tspiter->getRef());
				tideCalcPtr->getStationLocation(tspiter->getRef(), &reflat1, &reflon1);
				refdist1 = distanceBetween(reflat0, reflon0, tsp->getLat(), tsp->getLon()); // new
				refdist2 = distanceBetween(reflat1, reflon1, tsp->getLat(), tsp->getLon()); // old
				debugf(2, "  REJECT - already in the list %f vs %f\n", refdist1, refdist2);
				// XXX choose whether to reject the new one or delete the existing one
				// Remove the one which is furthest away
				// Could also deliberately choose the one with the same reference station
				// ref station used by the nearest suborbinate station
				// (eg. if this tidal diamond is close to Dundee and Dundee is referenced to Aberdeen)
				if (refdist2 > refdist1)
				{
					// Remove the old one it is further away
					// Could use replace but then the append below would have to be skipped
					// so easiest to just remove the old one here and fall through to append
					debugf(2, "    remove old one\n");
					tidalStreamList.removeRef(tspiter);
				}
				else
				{
					// Ignore the new one it is further away
					debugf(2, "    ignore new one\n");
					delete tsp;
					tsp = 0;
				}
				break;
			}
		}
		if (tsp == 0)
			continue;
		tidalStreamList.append(tsp);
	}
	file.close();
	return true;
}


void
DisplayWindow::loadTidalData()
{
#ifndef USE_UNZIP
	QDir dir(dirname);
	QStringList c1list = dir.entryList("*C10.C1");
	QStringList t1list = dir.entryList("*T10.T1");
		loadTidalLevelFile(dirname+"/"+(*diriter));
		loadTidalLevelFile(dirname+"/"+(*diriter));
#endif
	debugf(1, "loadTidalData\n");

	// Find all the files for year 2010 (have last two digits 10)
	// Names BAnnTyy.T1 and BAnnCyy.C1
	QString c1zip(DEFAULT_TIDE_DATA_DIR+DIRSEPSTR+DEFAULT_TIDEC1_ZIP);
	QString t1zip(DEFAULT_TIDE_DATA_DIR+DIRSEPSTR+DEFAULT_TIDET1_ZIP);

	ZipDir c1zipdir(c1zip);
	ZipDir t1zipdir(t1zip);

	QStringList c1list = c1zipdir.entryList("*C10.C1");
	QStringList t1list = t1zipdir.entryList("*T10.T1");

	QStringList::Iterator diriter;

	// Read the Tide Levels files (*.T1) containing mean high/low water
	tidalLevelList.clear();
	for (diriter = t1list.begin(); diriter != t1list.end(); ++diriter)
	{
		loadTidalLevelFile(t1zip, DEFAULT_TIDE_ZIP_PASSWORD, *diriter);
	}

	// Read the Tidal Streams files (*.C1) containing tidal diamonds
	tidalStreamList.clear();
	for (diriter = c1list.begin(); diriter != c1list.end(); ++diriter)
	{
		loadTidalStreamFile(c1zip, DEFAULT_TIDE_ZIP_PASSWORD, *diriter);
	}

	debugf(1, "Final list of Tidal Level stations on this chart:\n");
	for ( TidalLevel *tlpiter = tidalLevelList.first(); tlpiter; tlpiter = tidalLevelList.next() )
	{
		debugf(2, "  %s at %f %f\n", (const char*)tlpiter->getName(), tlpiter->getLat(), tlpiter->getLon());
	}

	debugf(1, "Final list of Tidal Streams on this chart:\n");
	for ( TidalStream *tspiter = tidalStreamList.first(); tspiter; tspiter = tidalStreamList.next() )
	{
		debugf(2, "  %s at %f %f ref %s\n", (const char*)tspiter->getName(), tspiter->getLat(), tspiter->getLon(), (const char*)tspiter->getRef());
	}
}


/* ----------------------------------------------------------------------------
 * plotTidalStreams is called whenever the time slider is moved or date changed
 * so recalculate all the tidal streams in the list and plot the new arrows
 */
void
DisplayWindow::plotTidalStreams()
{
	TidalStream *tsp;
	double jtime;
	float tideHeight;
	double jtimeHW;
	float lat, lon;
	float bearing, rate, length;
	double minsFromHW;
	double lunarPhaseFraction;

	jtime = slider_jtime + slider_offset;

	debugf(1, "plotTidalStreams at %s\n", jctime(jtime));

	// Find how far through the moon's cycle we are
	lunarPhaseFraction = moonCalcPtr->fractionFromSpringToNeap(jtime);

	// Remove all previously plotted arrows and refresh those screen areas
	qctimage->unplotArrows();

	// Calculate new direction of all arrows and plot them
	for (tsp = tidalStreamList.first(); tsp; tsp = tidalStreamList.next())
	{
		debugf(1, "plot TidalStream %s\n", (const char*)tsp->getName());
		// Where is this tidal stream
		lat = tsp->getLat();
		lon = tsp->getLon();
		// Which port does this tidal stream reference
		QString refstation = tsp->getRef();
		// Find the port's nearest HW time
		tideCalcPtr->findNearestHighWater(refstation, jtime, &tideHeight, &jtimeHW);
		// Find the bearing and rate of the stream at that time in the cycle
		minsFromHW = jtime - jtimeHW;
		tsp->getStreamMinsFromRefAndMoon(minsFromHW, lunarPhaseFraction, &bearing, &rate);
		length = rate * ARROW_SCALE;
		qctimage->plotArrow(lat, lon, bearing, length);
	}
}


void
DisplayWindow::plotTidalLevels()
{
	TidalLevel *tlp;
	double jtime;
	float lat, lon;
	float lowtide, hightide, tideHeight;

	jtime = slider_jtime + slider_offset;

	debugf(1, "plotTidalLevels at %s\n", jctime(jtime));

	// Remove all previously plotted tides and refresh those screen areas
	qctimage->unplotTides();

	// Calculate new direction of all arrows and plot them
	for (tlp = tidalLevelList.first(); tlp; tlp = tidalLevelList.next())
	{
		// Where is this tidal stream
		lat = tlp->getLat();
		lon = tlp->getLon();
		// What is the tidal range right now
		// XXX should interpolate for fraction between spring and neap
		lowtide = tlp->getMLWS();
		hightide = tlp->getMHWS();
		// Find the port's tide right now
		tideCalcPtr->findTide(tlp->getName(), jtime, &tideHeight);
		tlp->setCurrentLevel(tideHeight); // used by getCurrentLevel later in context menu
		debugf(1, "plot TidalLevel %s = %f [%f .. %f]\n", (const char*)tlp->getName(), tideHeight, lowtide, hightide);
		qctimage->plotTide(lat, lon, lowtide, hightide, tideHeight);
	}
}


/* ----------------------------------------------------------------------------
 * Change the starting date
 */
void
DisplayWindow::setDate(int Y, int M, int D, int h, int m)
{
	slider_jtime = date_to_jtime(Y, M, D, h, m);
	slider_offset = 0.0;
	int newpos = 0; // start at the left hand side
	int curval = slider->value();
	slider->setValue(newpos);
	// if slider value is unchanged then signal not emitted so call slot manually
	if (curval==newpos)
		slider_moved(newpos);
}


void 
DisplayWindow::editDate()
{
	int Y, M, D, h;
	double m;
	jtime_to_date(slider_jtime + slider_offset, &Y, &M, &D, &h, &m);
	QDate startdate(Y, M, D);
	QDate dat = CalendarDialog::getDate(this, startdate, QPoint());
	// it returns the start date if dialog rejected
	if (dat == startdate)
		return;
#if 0 
	bool ok;
	QDate dat = QDate::currentDate(), min, max;
	dat = InputDialog::getDate("Date", "Choose date",
		startdate, min, max, QDateEdit::DMY, &ok, this, "dateedit");
	if (!ok)
		return;
#endif
	setDate(dat.year(), dat.month(), dat.day(), 0, 0);
}


/* ----------------------------------------------------------------------------
 * Edit the application preferences
 */
void
DisplayWindow::editPrefs()
{
	// Check if already open and if so just bring it to foreground
	if (KAutoConfigDialog::showDialog("configdialog"))
		return;

	// Create a new dialog (name must match the one above)
	KAutoConfigDialog *dialog = new KAutoConfigDialog(this, "configdialog", KDialogBase::Tabbed, settings);

	// Add each "page" of settings
	ConfigWidget *configwidget = new ConfigWidget(0, "configwidget");
	//dialog->addPage(configwidget, "Page Name", "settings_group", "icon name");
	dialog->addPage(configwidget, "Display Preferences", "General", "");

	// Reload cfg_* variables when settings are changed
	connect(dialog, SIGNAL(settingsChanged()), this, SLOT(loadSettings()));

	dialog->show();
}


/* ----------------------------------------------------------------------------
 * Save, called from menu; saves the loaded map as a PNG image.
 */
void
DisplayWindow::saveMap()
{
	QString filename = QFileDialog::getSaveFileName(QString::null, "*.png", this, "savedialog");
	if (!filename.isEmpty())
	{
		if (!filename.endsWith(".png", false))
			filename += ".png";
		bool saved;
		QApplication::setOverrideCursor(waitCursor);
		saved = qctimage->save(filename, "PNG");
		QApplication::restoreOverrideCursor();
		if (!saved)
			QMessageBox::warning(this, progname, "Error saving image");
	}
}


void
DisplayWindow::saveScreen()
{
	QString filename = QFileDialog::getSaveFileName(QString::null, "*.png", this, "savedialog");
	if (!filename.isEmpty())
	{
		if (!filename.endsWith(".png", false))
			filename += ".png";
		bool saved;
		QApplication::setOverrideCursor(waitCursor);
		// Determine the region visible on screen
		int xx, yy, ww, hh;
		xx = qctimage->contentsX();
		yy = qctimage->contentsY();
		ww = qctimage->visibleWidth();
		hh = qctimage->visibleHeight();
		// Render it into a pixmap of the same dimensions (depth will be same as screen)
		QPixmap pixmap(ww, hh);
		QPainter painter(&pixmap);
		painter.setViewport(0,0,ww,hh);   // device coordinate system (pixmap)
		painter.setWindow(xx,yy,ww,hh);   // logical coordinate system (map pixels)
		qctimage->render(&painter, xx, yy, ww, hh);
		// Save the pixmap
		saved = pixmap.save(filename, "PNG");
		QApplication::restoreOverrideCursor();
		if (!saved)
			QMessageBox::warning(this, progname, "Error saving image");
	}
}


/* ----------------------------------------------------------------------------
 * Print, called from menu; prints the loaded map.
 */
bool
DisplayWindow::printMapSection(QRect section)
{
	float image_aspect_ratio, paper_aspect_ratio;
	QPrinter printer( QPrinter::HighResolution );

	int xx, yy, ww, hh;
	xx = section.x();
	yy = section.y();
	ww = section.width();
	hh = section.height();

	// Choose a default orientation suitable for image aspect ratio
	image_aspect_ratio = (float)ww / hh;
	if (image_aspect_ratio > 1.0)
		printer.setOrientation(QPrinter::Landscape);
	else
		printer.setOrientation(QPrinter::Portrait);
	// Graphs are usually in colour
	printer.setColorMode(QPrinter::Color);
	// Ignore "printable" margins and start at 0,0
	printer.setFullPage(TRUE);

	// Pop up print dialog asking for destination etc.
	if (printer.setup(this))
	{
		QApplication::setOverrideCursor(waitCursor);
		QPainter painter(&printer);
		if( !painter.isActive() ) // starting printing failed
		{
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, "Print", "Error initialising printer");
			return false;
		}
		// Get actual page size and place "paper" inset by 2cm margins
		QPaintDeviceMetrics metrics(painter.device());
		int dpiy = metrics.logicalDpiY();
		int margin = (int) ( (PRINTER_MARGIN_CM/2.54)*dpiy ); // 1 cm margins
		int paper_left = margin;
		int paper_top = margin;
		int paper_width = metrics.width() - 2*margin;
		int paper_height = metrics.height() - 2*margin;
		paper_aspect_ratio = (float)paper_width / paper_height;
		debugf(1, "logical dpi %d margin %d\n", dpiy, margin);
		debugf(1, "image_aspect_ratio=%f paper_aspect_ratio=%f\n", image_aspect_ratio, paper_aspect_ratio);
		debugf(1, "image rect %d,%d %dx%d\n", xx, yy, ww, hh);
		debugf(1, "paper rect %d,%d %dx%d\n", paper_left, paper_top, paper_width, paper_height);
		// If paper is wider than image then offset the left edge
		// otherwise offset the top edge
		if (paper_aspect_ratio > image_aspect_ratio)
		{
			int new_width = NINT(paper_height * image_aspect_ratio);
			paper_left += (paper_width - new_width)/2;
			paper_width = new_width;
			debugf(1, "pap>img, paper now %d,%d %dx%d\n", paper_left, paper_top, paper_width, paper_height);
		}
		else
		{
			int new_height = NINT(paper_width / image_aspect_ratio);
			paper_top += (paper_height - new_height)/2;
			paper_height = new_height;
			debugf(1, "pap<img, paper now %d,%d %dx%d\n", paper_left, paper_top, paper_width, paper_height);
		}

		QRect view( paper_left, paper_top, paper_width, paper_height );
		painter.save();
		painter.setViewport(view);   // device coordinate system (paper)
		painter.setWindow(section);  // logical coordinate system (map pixels)
		qctimage->render(&painter, xx, yy, ww, hh);
		painter.restore();

		QString text(jctime(slider_jtime+slider_offset));
		//QRect textbbox = painter.boundingRect(xx, yy, 0, 0, AlignLeft|AlignTop, text);
		//printf("paper_width %d ww %d text width %d\n", paper_width, ww, textbbox.width());
		//textbbox.setSize(textbbox.size() / (textbbox.size().width()/paper_width/0.25));
		//textbbox.setSize(textbbox.size() / 2);
		//painter.drawText(textbbox, AlignLeft|AlignTop, text);
		painter.drawText(margin, margin, text);
		QApplication::restoreOverrideCursor();
	}
	return true;
}


bool
DisplayWindow::printMap()
{
	QRect section(0, 0,
		qctimage->contentsWidth(), qctimage->contentsHeight());
	return printMapSection(section);
}


bool
DisplayWindow::printScreen()
{
	QRect section(qctimage->contentsX(), qctimage->contentsY(),
		qctimage->visibleWidth(), qctimage->visibleHeight());
	return printMapSection(section);
}


/* ----------------------------------------------------------------------------
 * Quit, called from menu
 */
void
DisplayWindow::quit()
{
	qApp->quit();
}


/* ----------------------------------------------------------------------------
 * Display About box
 */
void
DisplayWindow::about()
{
	QString msg=QString("xqct 1.01 (c) 2010 Andrew Brooks <arb@sat.dundee.ac.uk>\n"
		"\n"
		"This program displays QuickChart maps and tidal information");
    QMessageBox::about( this, progname, msg);
}

void
DisplayWindow::helpmanual()
{
	static HelpDialog *helpdialog = new HelpDialog(this, "xqct.html");
	helpdialog->show();
}
