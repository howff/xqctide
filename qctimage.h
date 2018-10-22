/* > qctimage.h
 * 1.00 arb
 */

#ifndef QCTIMAGE_H
#define QCTIMAGE_H

#include <qdatetime.h>    // for QTime
#include <qscrollview.h>  // base class
#include <qpixmap.h>
#include <qimage.h>
#include <qpointarray.h>

class QPainter;
class QCT;


class ArrowPlot
{
public:
	ArrowPlot(int x, int y, float bearing, float length);
	~ArrowPlot();
	void plot(QPainter *);
	QRect rect() const;
private:
	// constant data could be made static to the class
	int stemwidth;
	bool filled;
	// the array of transformed points and a quick-access bounding box
	QPointArray points;
	QRect boundingBox;
};


class TidePlot
{
public:
	TidePlot(int x, int y, float min, float max, float currently);
	~TidePlot();
	void plot(QPainter *);
	QRect rect() const;
private:
	// constant data could be made static to the class
	int dimension;
	// the array of transformed points and a quick-access bounding box
	QPointArray points;
	QRect boundingBox;
};


class QCTImage : public QScrollView
{
	Q_OBJECT
public:
	QCTImage(QWidget *parent);
	~QCTImage();
public:
	// Load and unload a QCT
	void unload();
	bool load(QString filename, int scalefactor = 1);
	// Save QCT as a PNG
	bool save(QString filename, const char *fmt);
	// The actual QCT object so it can be queried for name etc.
	QCT *getQct() { return qct; }
	// Plotting (only to be called when printing)
	void render(QPainter *painter, int cx, int cy, int cw, int ch);
	// Plotting arrows onto the image
	void unplotArrows();
	void plotArrow(float lat, float lon, float bearing, float length);
	// Plotting arrows onto the image
	void unplotTides();
	void plotTide(float lat, float lon, float min, float max, float currently);
	// Scrolling
	bool scrollToLatLon(double lat, double lon);
	bool latLonOfCenter(double *lat, double *lon);
signals:
	void location(double lat, double lon, long val);
	void mouseWheel(Qt::Orientation orient, int delta);
	void contextMenu(double lat, double lon);
protected:
	void drawContents(QPainter *painter, int cx, int cy, int cw, int ch);
	void contentsContextMenuEvent(QContextMenuEvent *event);
	void contentsMouseMoveEvent(QMouseEvent *event);
	void contentsMousePressEvent(QMouseEvent *event);
	void contentsMouseReleaseEvent(QMouseEvent *event);
	void contentsWheelEvent(QWheelEvent *event);
	void showEvent(QShowEvent*);
private slots:
	void keepFloating();
private:
	// To support panning:
	bool dragging, floating;
	QPoint dragStartMousePos, dragStartContentsPos, dragDelta;
	QTime lastMoveTime;
	// The image itself
	QImage image;
	QCT *qct;
	// Initialisation only
	int startx, starty;
	// List of arrows to be overlaid
	QPtrList<ArrowPlot> arrowList;
	QPtrList<TidePlot>  tideList;
};


#endif //!GDALIMAGE_H
