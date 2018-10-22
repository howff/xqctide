/* > qctcollection.cpp
 * 1.01 arb Wed Feb 23 10:45:35 GMT 2011 - error check openFilename
 * 1.00 arb Tue Jul 20 00:18:47 BST 2010
 */

static const char SCCSid[] = "@(#)qctcollection.1.01 (C) 2010 arb QCT file collection";


/*
 * To do:
 * Cache results in a file so faster next time.
 */

#include <qapplication.h>  // for cursor
#include <qdir.h>
#include <qstring.h>
#include "satlib/dundee.h" // for debugf
#include "osmap/qct.h"
#include "osmap/inpoly.h"
#include "qctcollection.h"


/* -------------------------------------------------------------------------
 * This is each individual map in the collection, containing the map outline.
 */
QCTCollectionMap::QCTCollectionMap(const QString &fname)
{
	filename = fname;

	numpoints = 0;
	degreesPerPixel = 0;
	// description, chartname, scale should be null already

	qct = new QCT();
	// pass headeronly=true to prevent reading whole image
	if (qct->openFilename(filename, true))
	{
		// Sample QCT has these fields:
		// Title="Ordnance Survey GB Route Planner", Name="OS GB Route Planner 1:1M", Identifier=""
		// Title="RIVER TAY", Name="BA1481 RIVER TAY", Identifier="BA1481"
		description = qct->getName();       // eg. "BA1481 RIVER TAY"
		chartname   = qct->getIdentifier(); // eg. "BA1481" or empty
		scale = QString::null;             // XXX 
		degreesPerPixel = qct->getDegreesPerPixel(); // XXX slow?

		numpoints = qct->getOutlineSize();
		if (numpoints > 2)
		{
			intpoints = new unsigned int[numpoints][2];
			int ii;
			double lat, lon;
			for (ii=0; ii<numpoints; ii++)
			{
				qct->getOutlinePoint(ii, &lat, &lon);
		// Convert all real numbers to positive integers
#define LAT_TO_INT(L) (unsigned int)((L+90.0) * 1e3)
#define LON_TO_INT(L) (unsigned int)((L+180.0) * 1e3)
				intpoints[ii][0] = LON_TO_INT(lon);
				intpoints[ii][1] = LAT_TO_INT(lat);
			}
		}

		qct->closeFilename();
	}
	else
	{
		fprintf(stderr, "Warning: cannot load '%s' into QCT collection\n", (const char*)fname);
	}
	delete qct;
}


QCTCollectionMap::~QCTCollectionMap()
{
	if (numpoints > 2)
		delete [] intpoints;
}


QString
QCTCollectionMap::getFilename() const
{
	return filename;
}


QString
QCTCollectionMap::getLabel() const
{
	return description;
}


QString
QCTCollectionMap::getScale() const
{
	return scale;
}


float
QCTCollectionMap::getDegreesPerPixel() const
{
	return degreesPerPixel;
}


bool
QCTCollectionMap::containsLatLon(double lat, double lon)
{
	int rc, intlat, intlon;

	// Need at least 3 points to make a polygon!
	if (numpoints < 3)
		return false;

	// Convert all real numbers to positive integers
	intlat = LAT_TO_INT(lat);
	intlon = LON_TO_INT(lon);

	// Fast check if point within polygon
	rc = inpoly(intpoints, numpoints, intlon, intlat);

	return (rc ? true : false);
}


/* -------------------------------------------------------------------------
 * This is the thread which runs in the background collecting map outlines.
 */
QCTCollectionWorker::QCTCollectionWorker()
{
	qctlistptr = 0;
}


QCTCollectionWorker::~QCTCollectionWorker()
{
}


void
QCTCollectionWorker::setPaths(QStringList paths)
{
	mapPaths = paths;
}


void
QCTCollectionWorker::setMapList(QPtrList<QCTCollectionMap> *qctlistp)
{
	qctlistptr = qctlistp;
}


// Call this when you have an interactive requirement for the results
// it prevents the background thread sleeping nicely so it's faster
void
QCTCollectionWorker::hurryUp()
{
	stillBackgroundThread = false;
}


void
QCTCollectionWorker::run()
{
	if (qctlistptr == 0 || mapPaths.count() == 0)
		return;

	// Be nice
	stillBackgroundThread = true;

	QStringList::Iterator pathiter;
	for (pathiter = mapPaths.begin(); pathiter != mapPaths.end(); ++pathiter)
	{
		debugf(2, "  QCTCollection dir %s\n", (const char*)*pathiter);

		QDir dir(*pathiter);
		QStringList files = dir.entryList("*.qct *.QCT");

		QStringList::Iterator fileiter;
		for (fileiter = files.begin(); fileiter != files.end(); ++fileiter)
		{
			debugf(3, "    QCTCollection file %s\n", (const char*)*fileiter);
			QCTCollectionMap *map = new QCTCollectionMap(*pathiter + "/" + *fileiter);
			qctlistptr->append(map);

			// The user can tell us the hurry up (when the collection is
			// requested before we've finished collecting it) but until
			// then we sleep to ensure the foreground thread gets some CPU.
			if (stillBackgroundThread)
				QThread::msleep(100);
		}
	}
}


/* -------------------------------------------------------------------------
 * This is the collection of maps, this is the object which the user creates.
 */
QCTCollection::QCTCollection(const QString &qctdir)
{
	qctlist.setAutoDelete(true);
	if (!qctdir.isEmpty())
		setPath(qctdir);
}


QCTCollection::~QCTCollection()
{
	if (worker.running())
	{
		worker.terminate();
		worker.wait();
	}
}


void
QCTCollection::setPath(const QString &path)
{
	setPaths(QStringList(path));
}


void
QCTCollection::setPaths(QStringList paths)
{
	mapPaths = paths;
	collectMaps();
}


void
QCTCollection::collectMaps()
{
	qctlist.clear();

	// Tell the worker where to look and where to put the results
	worker.setPaths(mapPaths);
	worker.setMapList(&qctlist);

	// Start it running in the background thread
	worker.start();
}


QStringList
QCTCollection::mapsListAtLatLon(double lat, double lon)
{
	// Collection of maps must have completed before we can continue
	QApplication::setOverrideCursor(Qt::waitCursor);
	worker.hurryUp();
	worker.wait();
	QApplication::restoreOverrideCursor();

	// Create a table mapping resolution to map name
	typedef QMap<float,QString> mapTable_t;
	mapTable_t maptable;

	// Iterate through all maps in the collection looking for those which
	// cover the point of interest and add the map name to a table which is
	// indexed by resolution.
	debugf(1,"QCTCollection::mapsListAtLatLon query %f %f\n",lat,lon);
	for ( QCTCollectionMap *mapiter = qctlist.first(); mapiter; mapiter = qctlist.next() )
	{
		if (mapiter->containsLatLon(lat, lon))
		{
			debugf(2,"  match %s\n", (const char*)mapiter->getFilename());
			maptable[mapiter->getDegreesPerPixel()] = mapiter->getLabel();
		}
	}

	// Create a list of map names by extracting them from the table.
	// Note the items in the table are automatically sorted by the primary key
	// (the resolution) so we can simply pull out the sorted map names
	// ordered from close-up to zoomed-out, handy for the menu.
	QStringList list;
	mapTable_t::Iterator iter;
	for ( iter = maptable.begin(); iter != maptable.end(); ++iter )
	{
		list << iter.data();
	}
	return(list);
}


QString
QCTCollection::getFilenameForMap(const QString &mapname)
{
	QString fn;
	for ( QCTCollectionMap *mapiter = qctlist.first(); mapiter; mapiter = qctlist.next() )
	{
		if (mapiter->getLabel() == mapname)
		{
			fn = mapiter->getFilename();
			break;
		}
	}
	return fn;
}
