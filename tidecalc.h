/* > tidecalc.h
 * 1.00 arb 
 */

#ifndef TIDECALC_H
#define TIDECALC_H


#include <qstring.h>
#include <qdict.h>


#define TIDECALC_DAYS           8  // for 8 days:
#define TIDECALC_INTERVAL_MINS 15  // every 15 minutes
#define TIDECALC_PERDAY     (24*4) // 4 per hour at 15 min intervals
#define TIDECALC_POINTS (TIDECALC_DAYS*TIDECALC_PERDAY)
#define TIDECALC_HW_POINTS (TIDECALC_DAYS*3) // at most 3 high tides per day


class TCD
{
public:
	TCD();
	~TCD();
	bool load(const QString &harmonicsFilename);
	bool setStation(const QString &station); // can be called multiple times to get next matching station
	bool getStationLocation(double *lat, double *lon);
private:
	int currentStationNum;
	QString currentStationGivenName;
	bool tcdOk;
	bool stationOk;
};


class TideCalcStation
{
public:
	TideCalcStation(const QString &station, double startjtime);
	~TideCalcStation();
	//int setStationTime(const QString &station, const QDateTime &datetime);
	int findTide(double jtime, float *tideheight);
	int findNearestHighWater(double jtime, float *height, double *jtimeHW);
private:
	void initialiseTideTimes(double jtime);
	int calculateTideTimes();
	int calculateHighWaterTimes();
private:
	QString station;
	double starttime, endtime;
	double springHWtime, neapHWtime;
	float height[TIDECALC_POINTS];
	double jtime_hw[TIDECALC_HW_POINTS];
};


class TideCalc
{
public:
	TideCalc(const QString &harmonicsFile = QString::null);
	~TideCalc();
public:
	bool loadTideDatabase(const QString &filename);
	bool getStationLocation(const QString &station, double *lat, double *lon);
	int findTide(const QString &station, double jtime, float *tideheight);
	int findNearestHighWater(const QString &station, double jtime, float *tideheight, double *jtimeHW);
private:
	QString harmonicsFilename;
	TCD tidedatabase;
	typedef QDict<TideCalcStation> TideCalcStationDict;
	TideCalcStationDict stationdict;
};


#define SYNODIC_MONTH_IN_DAYS 29.53058868

class MoonCalc
{
public:
	MoonCalc();
	float daysSinceNew(double jtime);
	double minsSinceNew(double jtime);
	double fractionFromSpringToNeap(double jtime);
private:
	double lastjtime;
	float lastage;
};



#endif /* !TIDECALC_H */
