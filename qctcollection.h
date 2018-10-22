/* > qctcollection.h
 * 1.00 arb
 */

#ifndef QCTCOLLECTION_H
#define QCTCOLLECTION_H

#include <qstringlist.h>
#include <qthread.h>
class QCT; //#include "osmap/qct.h"


/*
 * This class is private not to be used externally.
 * It encapsulates the information in a single map regarding which region
 * it covers so we can query it quickly by location.
 */
class QCTCollectionMap
{
public:
	QCTCollectionMap(const QString &filename);
	~QCTCollectionMap();
	bool containsLatLon(double lat, double lon);
	QString getFilename() const;
	QString getLabel() const;
	QString getScale() const;
	float getDegreesPerPixel() const;
private:
	QString filename;
	QString chartname;
	QString description;
	QString scale;
	float degreesPerPixel;
	int numpoints;
	unsigned int (*intpoints)[2]; // ie. int[][2], coords passed to inpoly()
	QCT *qct;
};


/*
 * This class is private not to be used externally.
 * It collects the map list and bounding boxes in the background.
 * The run() method is a separate thread, you must wait() on it
 * when you need it to have finished.
 */
class QCTCollectionWorker : public QThread
{
public:
	QCTCollectionWorker();
	~QCTCollectionWorker();
	void setPaths(QStringList);
	void setMapList(QPtrList<QCTCollectionMap>*);
	void run(); // this will run in the "background" but must sleep to relinquish CPU
	void hurryUp();
private:
	bool stillBackgroundThread;             // true in background, false for faster foreground
	QStringList mapPaths;                   // directories in which to look for maps
	QPtrList<QCTCollectionMap> *qctlistptr; // pointer to the QCTCollection list of maps
};


/*
 * This class is a collection of maps which can be queried by location
 * to find out which maps contain the given location.
 */
class QCTCollection
{
public:
	QCTCollection(const QString &qctdir);
	~QCTCollection();
	void setPath(const QString &);
	void setPaths(QStringList);
	void collectMaps();
	QStringList mapsListAtLatLon(double lat, double lon);
	QString getFilenameForMap(const QString &mapname);
private:
	QStringList mapPaths;                // directories in which to look for maps
	QCTCollectionWorker worker;          // populates qctlist in the background
	QPtrList<QCTCollectionMap> qctlist;  // list of maps
};


#endif // !QCTCOLLECTION_H
