/* > qctimage.cpp
 * 1.00 arb Thu May 27 09:18:38 BST 2010
 */

static const char SCCSid[] = "@(#)qctimage.cpp  1.00 (C) 2010 arb QCT image scrollview";

/*
 * QCTImage is a class to display a QCT (QuickChart) image.
 * It is a subclass of QScrollView so it can be used like a widget
 * that displays an image with scrollbars.
 */

/*
 * Configuration:
 * Define STOP_MAP_MOVING_AFTER_MSEC as number of milliseconds to wait
 * after a mouseMove event to get a mouseButtonReleased event.  If moving
 * within MSEC period then assume user wants map to continue moving,
 * otherwise user paused the mouse before releasing the button so wants
 * it to remain stationary.
 */
#define STOP_MAP_MOVING_AFTER_MSEC 50


#include <qapplication.h> // for OverrideCursor
#include <qcursor.h>      // for QCursor::pos
#include <qpainter.h>
#include <qtimer.h>

#include "satlib/dundee.h"
#include "osmap/qct.h"
#include "qctimage.h"


/* ---------------------------------------------------------------------------
 *
 */
ArrowPlot::ArrowPlot(int x, int y, float bearing, float length)
{
	stemwidth = 8;
	filled = true;

	QPointArray prepoints(8);
	prepoints[0] = QPoint(-stemwidth, 0);
	prepoints[1] = QPoint(-stemwidth, NINT(-length));
	prepoints[2] = QPoint(-stemwidth*2, NINT(-length));
	prepoints[3] = QPoint(0, NINT(-length)-stemwidth*2);
	prepoints[4] = QPoint(stemwidth*2, NINT(-length));
	prepoints[5] = QPoint(stemwidth, NINT(-length));
	prepoints[6] = QPoint(stemwidth, 0);
	prepoints[7] = QPoint(-stemwidth, 0);

	// Translate origin THEN rotate all the points
	QWMatrix wm;
	wm.translate(x, y);
	wm.rotate(bearing);
	points = wm.map(prepoints);

	// For speed extract the bounding box and keep it
	boundingBox = points.boundingRect();
}

ArrowPlot::~ArrowPlot()
{
}

QRect
ArrowPlot::rect() const
{
	return boundingBox;
}

void
ArrowPlot::plot(QPainter *painter)
{
	if (filled)
		painter->drawPolygon(points);
	else
		painter->drawPolyline(points);
}


/* ---------------------------------------------------------------------------
 *
 */
TidePlot::TidePlot(int x, int y, float min, float max, float currently)
{
	dimension = 5;

	// painter coords go from top left but level is up from bottom
	int level = NINT((currently-min)/(max-min)*dimension*4);

	QPointArray prepoints(7);
	prepoints[0] = QPoint(-dimension, -dimension*2);
	prepoints[1] = QPoint(-dimension,  dimension*2);
	prepoints[2] = QPoint( dimension,  dimension*2);
	prepoints[3] = QPoint( dimension, -dimension*2);
	prepoints[4] = QPoint(-dimension, -dimension*2);
	prepoints[5] = QPoint(-dimension,  dimension*2-level);
	prepoints[6] = QPoint( dimension,  dimension*2-level);
	debugf(1,"TIDEPLOT %f < %f < %f =  %d < %d(%f) < %d\n",
		min,currently,max, -dimension*2,-dimension*2+level,((currently-min)/(max-min)*dimension*4),dimension*2);

	// Translate origin to move all the points
	QWMatrix wm;
	wm.translate(x, y);
	points = wm.map(prepoints);

	// For speed extract the bounding box and keep it
	boundingBox = points.boundingRect();
}


TidePlot::~TidePlot()
{
}


QRect
TidePlot::rect() const
{
	return boundingBox;
}


void
TidePlot::plot(QPainter *painter)
{
	painter->drawPolyline(points);
}


/* ---------------------------------------------------------------------------
 *
 */
QCTImage::QCTImage(QWidget *parent) : QScrollView(parent, "qcti", WNoAutoErase|WStaticContents|WPaintClever)
{
	qct = 0;
	dragging = floating = false;
	startx = starty = 0;

	arrowList.setAutoDelete(true);
	tideList.setAutoDelete(true);

	// Allow mouse move events through to contentsMouseMoveEvent
	viewport()->setMouseTracking(true);

	// Enable panning -- this only works if you try to drag an item
	// from outside the window (eg. from the Desktop) into the window
	// and hold it near the edge.  It's a nice effect though.
	//setDragAutoScroll(true);
	//viewport()->setAcceptDrops(true);
}


QCTImage::~QCTImage()
{
	unload();
}


void
QCTImage::unload()
{
	if (qct)
	{
		delete qct;
		qct = 0;
	}
	arrowList.clear();
	tideList.clear();
}


/*
 * Load a QCT, create a QImage, copy the image data from the QCT,
 * resize the internal dimensions of the scrollview, scroll to middle.
 */
bool
QCTImage::load(QString filename, int scalefactor)
{
	int ii, yy, width, height;

	qct = new QCT();
	QApplication::setOverrideCursor(waitCursor);
	if (!qct->openFilename((const char*)filename, false, scalefactor))
	{
		log_error_message("cannot read %s", (const char*)filename);
		return false;
	}
	QApplication::restoreOverrideCursor();

	// Create the internal image with the correct dimensions
	width = qct->getImageWidth();
	height = qct->getImageHeight();
	image.reset();
	image.create(width, height, 8, 256);
	debugf(1, "loaded QCT %d x %d (reduced %d) from %s\n", width, height, scalefactor, (const char*)filename);

	// Fill in the colourmap by querying the QCT colourmap
	for (ii=0; ii<256; ii++)
	{
		int R, G, B;
		qct->getColour(ii, &R, &G, &B);
		image.setColor(ii, qRgb(R, G, B));
	}

	// Fill in the image data by reading the QCT image data
	for (yy=0; yy<height; yy++)
	{
		memcpy(image.scanLine(yy), qct->getImage()+yy*width*sizeof(unsigned char), width*sizeof(unsigned char));
	}

	// Free the image memory from within the QCT object now we've copied it
	qct->unloadImage();

	// Tell the scrollview how big its image is
	resizeContents(width, height);

	// Scroll to the middle
	center(width/2, height/2);

	return true;
}


/*
 * Save the image to a file
 */
bool
QCTImage::save(QString filename, const char *fmt)
{
	return image.save(filename, fmt);
}


/* --------------------------------------------------------------------------
 * Paint the portion of the image onto the screen or printer.
 */
void
QCTImage::render(QPainter *painter, int cx, int cy, int cw, int ch)
{
	if (image.isNull())
	{
		//painter->eraseRect(area);
		return;
	}

	// Plot the visible part of the image
	QRect area(cx, cy, cw, ch);
	painter->drawImage(area.topLeft(), image, area);

	// Border and fill colour of arrows
	painter->save();
	painter->setPen(QPen(black, 1, SolidLine));   // ! use no thicker than 1
	painter->setBrush(QBrush(red, SolidPattern));

	// Get all the arrows to draw themselves
	for (ArrowPlot *arrow = arrowList.first(); arrow; arrow = arrowList.next())
	{
		debugf(1, "ARROW %d %d %d %d ",arrow->rect().x(), arrow->rect().y(),
			arrow->rect().width(), arrow->rect().height());
		if (arrow->rect().intersects(area))
		{
			debugf(1, "plot\n");
			arrow->plot(painter);
		}
		else
		{
			debugf(1, "invisible\n");
		}
	}
	painter->restore();

	// Border and fill colour of tides
	painter->save();
	painter->setPen(QPen(black, 2, SolidLine));
	painter->setBrush(QBrush(green, SolidPattern));

	// Get all the tide marks to draw themselves
	for (TidePlot *tideplot = tideList.first(); tideplot; tideplot = tideList.next())
	{
		debugf(1, "TIDEPLOT %d %d %d %d ", tideplot->rect().x(), tideplot->rect().y(),
			tideplot->rect().width(), tideplot->rect().height());
		if (tideplot->rect().intersects(area))
		{
			debugf(1, "plot\n");
			tideplot->plot(painter);
		}
		else
		{
			debugf(1, "invisible\n");
		}
	}
	painter->restore();
}


void
QCTImage::drawContents(QPainter *painter, int cx, int cy, int cw, int ch)
{
	render(painter, cx, cy, cw, ch);
}


/* --------------------------------------------------------------------------
 * Right-click context menu requested (event contains content coords)
 */
void
QCTImage::contentsContextMenuEvent(QContextMenuEvent *event)
{
	int mx = event->pos().x();
	int my = event->pos().y();
	int imagewidth = image.width();
	int imageheight = image.height();
	if (mx < 0 || my < 0 || mx > imagewidth || my > imageheight)
		return;

	double lat, lon;
	qct->xy_to_latlon(mx,my, &lat, &lon);
	debugf(1,"QCTImage::contentsContextMenuEvent %d, %d = %f, %f\n", mx, my, lat, lon);

	emit contextMenu(lat, lon);

	// XXX accept or ignore event?
	event->accept();
}


/* --------------------------------------------------------------------------
 * Mouse is moving over the map
 *  convert mouse position into latitude and longitude,
 *  if button was pressed then dragging is true so scroll the map
 *  (could test for button by looking at event->state instead?)
 * [NB. Only called if mouse tracking has been enabled.]
 */
void
QCTImage::contentsMouseMoveEvent(QMouseEvent *event)
{
	// Do nothing if no image has been loaded
	if (image.isNull() || qct == 0)
		return;

	// Do nothing if point is not within image
	int mx = event->pos().x();
	int my = event->pos().y();
	int imagewidth = image.width();
	int imageheight = image.height();
	if (mx < 0 || my < 0 || mx > imagewidth || my > imageheight)
		return;

	double lat, lon;
	qct->xy_to_latlon(mx,my, &lat, &lon);
	//debugf(1,"QCTImage::contentsMouseMoveEvent %d, %d = %f, %f (TL %d %d)\n", mx, my, lat, lon, contentsX(), contentsY());

	// Could set val as colour of pixel at mx,my
	emit location(lat, lon, 0);

	// Panning
	if (dragging)
	{
		dragDelta = QPoint(dragStartMousePos - contentsToViewport(event->pos()));
		QPoint newpos = dragStartContentsPos + dragDelta;
		setContentsPos(newpos.x(), newpos.y());
		lastMoveTime.start();
		//debugf(1,"Mouse started at %5d %5d\n", dragStartMousePos.x(), dragStartMousePos.y());
		//debugf(1,"contents were at %5d %5d\n", dragStartContentsPos.y(), dragStartContentsPos.y());
		//debugf(1,"mouse now        %5d %5d\n", event->pos().x(), event->pos().y());
		//debugf(1,"delta            %5d %5d\n", delta.x(), delta.y());
		//debugf(1,"contents now at  %5d %5d\n", newpos.x(), newpos.y());
	}

	// XXX accept or ignore event?
	event->accept();
}


void
QCTImage::contentsMousePressEvent(QMouseEvent *event)
{
	// If not a plain left-button then might be a context menu so ignore
	if (event->button() != Qt::LeftButton)
	{
		event->ignore();
		return;
	}

	dragging = true;
	floating = false;
	dragStartMousePos = contentsToViewport(event->pos());
	dragStartContentsPos = QPoint(contentsX(), contentsY());

	// Display dragging-hand cursor
	QApplication::setOverrideCursor(Qt::PointingHandCursor);

	// XXX accept or ignore event?
	event->accept();
}


void
QCTImage::contentsMouseReleaseEvent(QMouseEvent *event)
{
	// Prevent any future mouseMoveEvents from continuing to drag the map
	dragging = false;

	// If mouse was moving recently (not paused over one spot) then assume the
	// user wants the map to continue moving in the same direction, ie floating
	if (lastMoveTime.elapsed() < STOP_MAP_MOVING_AFTER_MSEC)
	{
		floating = true;
		keepFloating();
	}
	else
	{
		floating = false;
	}

	// Remove dragging-hand cursor
	// XXX may not have been set so shouldn't unset without checking first
	QApplication::restoreOverrideCursor();

	// XXX accept or ignore event?
	event->accept();
}


void
QCTImage::keepFloating()
{
	// Keep moving smoothly in the same direction
	scrollBy(dragDelta.x() / 4, dragDelta.y() / 4);

	// Come back later and move further
	if (floating)
		QTimer::singleShot(100, this, SLOT(keepFloating()));
}


void
QCTImage::contentsWheelEvent(QWheelEvent *event)
{
	// If you only want the wheel event to be effective when a Shift/Ctrl/Alt key is pressed:
	//if (event->state() & Qt::KeyButtonMask)

	{
		emit mouseWheel(event->orientation(), event->delta());
	}

	// XXX accept or ignore event, ignore to allow scrolling to work?
	event->accept();
}


/* --------------------------------------------------------------------------
 * Delete all info about plotted items ready to have new items plotted
 * (so doesn't bother to update the screen)
 */
void
QCTImage::unplotArrows()
{
	// Only erase the visible bits
	QRect vis(contentsX(), contentsY(), contentsWidth(), contentsHeight());

	// Erase all the old arrows before their shape changes to new bbox
	for (ArrowPlot *arrow = arrowList.first(); arrow; arrow = arrowList.next())
	{
		QRect arrect = arrow->rect();
		if (arrect.intersects(vis))
			updateContents(arrect);
	}
	// Empty the list
	arrowList.clear();
}


void
QCTImage::unplotTides()
{
	// Only erase the visible bits
	QRect vis(contentsX(), contentsY(), contentsWidth(), contentsHeight());

	// Erase all the old arrows before their shape changes to new bbox
	for (TidePlot *tideplot = tideList.first(); tideplot; tideplot = tideList.next())
	{
		QRect tidrect = tideplot->rect();
		if (tidrect.intersects(vis))
			updateContents(tidrect);
	}
	// Empty the list
	tideList.clear();
}


/* --------------------------------------------------------------------------
 * Convert latitude and longitude into pixel coordinate
 * and plot.
 */
void
QCTImage::plotArrow(float lat, float lon, float bearing, float length)
{
	if (!qct)
		return;
	int x, y;
	qct->latlon_to_xy(lat, lon, &x, &y);
	ArrowPlot *arrow = new ArrowPlot(x, y, bearing, length);
	arrowList.append(arrow);

	debugf(1,"plot at %f %f = %d %d bearing %f length %f\n",lat,lon, x,y, bearing,length);

	updateContents(arrow->rect());
}


void
QCTImage::plotTide(float lat, float lon, float min, float max, float currently)
{
	if (!qct)
		return;
	int x, y;
	qct->latlon_to_xy(lat, lon, &x, &y);
	TidePlot *tideplot = new TidePlot(x, y, min, max, currently);
	tideList.append(tideplot);

	debugf(1,"plot at %f %f = %d %d min %f max %f currently %f\n",lat,lon, x,y, min,max,currently);

	updateContents(tideplot->rect());
}


/* --------------------------------------------------------------------------
 * Scroll so the given location is in the middle of the screen
 */
bool
QCTImage::scrollToLatLon(double lat, double lon)
{
	if (!qct)
		return false;

	int x, y;
	qct->latlon_to_xy(lat, lon, &x, &y);
	if (x<0 || y<0 || x>image.width()-1 || y>image.height()-1)
		return false;
	debugf(1,"scrollToLatLon %f %f -> %d %d\n", lat,lon, x,y);
	center(x, y);

	// If we've not been shown yet then the above center() won't work
	// so we take a note of the requested position until showEvent().
	startx = x;
	starty = y;

	return true;
}


void
QCTImage::showEvent(QShowEvent *ev)
{
	if (startx && starty)
	{
		center(startx, starty);
		startx = starty = 0;
	}
}


/* --------------------------------------------------------------------------
 * Return the coordinate of the middle of the visible part of the image.
 */
bool
QCTImage::latLonOfCenter(double *lat, double *lon)
{
	if (!qct)
		return false;

	int x, y;

	x = contentsX() + visibleWidth() / 2;
	y = contentsY() + visibleHeight() / 2;
	qct->xy_to_latlon(x, y, lat, lon);
	debugf(1,"latLonOfCenter %d %d -> %f %f\n",x,y, *lat,*lon);
	return true;
}
