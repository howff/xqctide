#ifndef XQCT_H
#define XQCT_H

#include "satlib/dundee.h"
#include <qmainwindow.h>
#include <qpixmap.h>


/*
 * Configuration:
 */


class QLabel;
class QPainter;
class QPopupMenu;
class QSlider;
//class QSettings;
class KConfig; // replaces QSettings
class QTimer;
class MRUMenu;

class QCTImage;
class TideCalc;
class MoonCalc;
class TidalLevel;
class TidalStream;
class QCTCollection;


class DisplayWindow: public QMainWindow
{
    Q_OBJECT
public:
	DisplayWindow();
	~DisplayWindow();

public slots:
	void loadMap();                                       // prompt for filename
	void loadNewMapFile(const QString &qctfile, const QString &metadata = QString::null);
	void loadMapFile(const QString &qctfile, const QString &metadata = QString::null);
	void setDate(int Y, int M, int D, int hh, int mm);
	void editDate();                                      // prompt for date
	void editPrefs();
	void changeZoom(int reductionFactor);                 // 1, 2, 4 or 8
	void saveMap();                                       // whole map without overlay
	void saveScreen();                                    // just the area displayed with overlay
	bool printMap();                                      // whole map (no longer used)
	bool printScreen();                                   // just the area displayed

private:
	bool loadTidalLevelFile(const QString &zipname, const QString &zippassword, const QString &filename);
	bool loadTidalStreamFile(const QString &zipname, const QString &zippassword, const QString &filename);

private slots:
	void loadSettings();
	void saveSettings();
	void saveMapMetadata();
	bool printMapSection(QRect);
	void slider_moved(int);
	void mouse_moved(double latN, double lonE);
	void mouse_wheel(Qt::Orientation orient, int delta);
	void context_menu(double latN, double lonE);
	void context_menu_map(int id);
	void context_menu_level(int id);
	void context_menu_stream(int id);
	void loadTidalData();                 // load C1,T1 from zip
	void showTidalStreamMenu();           // aboutToShow->populate the menu
	void tidalStreamMenuSelected(int id); // id is index into tidalStreamList
	void showTidalLevelMenu();            // aboutToShow->populate the menu
	void tidalLevelMenuSelected(int id);  // id is index into tidalLevelList
	void plotTidalStreams();
	void plotTidalLevels();
	void quit();
	void about();
	void helpmanual();

private:
	// Constructing the GUI
	QPopupMenu *editMenu, *viewMenu, *tidalLevelMenu, *tidalStreamMenu;
	QPopupMenu *mapContextMenu; //, *tidalLevelContextMenu, *tidalStreamContextMenu;
	QLabel *dateLabel, *latlonDegLabel, *latlonDMSLabel, *bngLabel;
	KConfig *settings; // was QSettings*
	MRUMenu *mruMenu;
	QSlider *slider;

	// The map itself
	QCTCollection *mapCollectionPtr;
	QString mapFilename;
	QCTImage *qctimage;

	// Tide calculations (via the external "tide" program)
	TideCalc *tideCalcPtr;
	MoonCalc *moonCalcPtr;

	// List of tidal streams on the currently-displayed map
	QPtrList<TidalLevel>  tidalLevelList;
	QPtrList<TidalStream> tidalStreamList;

	// Slider time at the left and minutes offset
	double slider_jtime;
	double slider_offset;

	// Configurable options
	bool cfg_TLmustBeOnChart, cfg_TLmustBeUnique;
	bool cfg_TSmustBeOnChart, cfg_TSmustBeUnique, cfg_TSmustHaveKnownRef;
	int cfg_zoomOutLevel;        // 1, 2, 4 or 8
};


#endif
