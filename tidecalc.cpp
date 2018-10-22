/* > tidecalc.cpp
 * 1.02 arb Sat Jul 10 17:11:42 BST 2010 - added TCD class.
 * 1.01 arb Sat Jul 10 16:45:23 BST 2010 - fix a bug in the Moon.
 * 1.00 arb Sun Jun 27 23:55:32 BST 2010
 */

static const char SCCSid[] = "@(#)tidecalc.cpp  1.02 (C) 2010 arb Tide Calculation via xtide";

/*
 * Calls the "tide" program (distributed by xtide) for the given station
 * with a range of 3 days either side of the given time. Caches the tide
 * states every 15 minutes so if called again can return an answer quickly.
 *
 * TCD is simply an interface to the tide database to lookup station names
 * and their locations. Actual tide calculations are done by the "tide"
 * program.
 *
 * TideCalc calculates tide times and high water (HW) times, and caches the
 * results for faster queries.  It also has a method to query the tide
 * database for a station location.  That's not cached but the most recent
 * location is remembered for faster repetetive queries.
 *
 * MoonCalc calculates the moon phase and caches the result for faster queries.
 */


/*
 * Configuration:
 * Define DEFAULT_HARMONICS_FILE as the initial tide database.
 */
#define DEFAULT_HARMONICS_FILE "harmonics-dwf-20091227-nonfree.tcd"


#include <stdio.h>
#include <qapplication.h>
#include <qdir.h>
#include "satlib/dundee.h" // for jtime stuff
#include "libtcd/tcd.h"    // for TCD
#include "tidecalc.h"


/* --------------------------------------------------------------------------
 * Tide Constituents Database
 *   Maintains static data regarding a single current station and keeps
 * the database open too.
 *   Call load first
 *   Call setStation as often as you like after opening
 *   then getStationLocation once a station has been successfully set.
 * NOTE: location is +/-180 positive East.
 */
TCD::TCD()
{
	tcdOk = stationOk = false;
	currentStationNum = -1;
}


TCD::~TCD()
{
	if (tcdOk)
		close_tide_db();
}


bool
TCD::load(const QString &harmonicsFilename)
{
	tcdOk = open_tide_db((const char*)harmonicsFilename);
	debugf(1,"TCD::load(%s) = %s\n",(const char*)harmonicsFilename,tcdOk?"ok":"error");
	return tcdOk;
}


bool
TCD::setStation(const QString &station)
{
	int rc;

	/* See if the current station has the same name */
	if ((currentStationNum >= 0) && (currentStationGivenName == station))
	{
		debugf(1, "TCD(%s) already set\n", (const char*)station);
		return true;
	}

	rc = find_station((const char*)station);
	if (rc == -1)
	{
		debugf(1, "TCD(%s) exact match failed, trying fuzzy\n", (const char*)station);
		// can be called multiple times to get next matching station
		rc = search_station((const char*)station);
		/* After using search_station you have to keep searching until you get
		 * a -1 in order to reset it for next time otherwise next time you'll
		 * get a -1 unexpectedly
		 */
		if (rc) { while (search_station((const char*)station) != -1) /* What to do with other matches? */ ; }
	}
	if (rc == -1)
	{
		stationOk = false;
		debugf(1, "TCD(%s) failed\n", (const char*)station);
	}
	else
	{
		stationOk = true;
		currentStationNum = rc;
		currentStationGivenName = station;
		debugf(1, "TCD(%s) OK (%d = %s)\n", (const char*)station, currentStationNum, get_station(currentStationNum));
	}
	return(stationOk);
}


/*
 * NOTE: returned location is +/-180 positive East.
 */
bool
TCD::getStationLocation(double *lat, double *lon)
{
	TIDE_STATION_HEADER hdr;
	*lat = *lon = 0.0;
	if (!get_partial_tide_record(currentStationNum, &hdr))
		return(false);
	*lat = hdr.latitude;
	*lon = hdr.longitude;
	debugf(1, "getStationLocation = %f %f\n", *lat, *lon);
	return(true);
}


/* --------------------------------------------------------------------------
 * TideCalcStation
 *   This object maintains a cache of tide heights for one station
 * for a period surrounding the desired time so that tide heights for
 * nearby times can be returned quickly.
 */
TideCalcStation::TideCalcStation(const QString &sta, double startjtime)
{
	debugf(1, "TideCalcStation(%s, %s)\n", (const char*)sta, jctime(startjtime));
	station = sta;
	
	springHWtime = neapHWtime = 0;
	for (int ii=0; ii<TIDECALC_POINTS; ii++)
		height[ii]=0;

	initialiseTideTimes(startjtime);

	calculateTideTimes();
	calculateHighWaterTimes();
}


TideCalcStation::~TideCalcStation()
{
}


void
TideCalcStation::initialiseTideTimes(double jtime)
{
	// XXX round down to a 15-minute boundary
	int Y, M, D, H;
	double min;
	jtime_to_date(jtime, &Y, &M, &D, &H, &min);
	min -= fmod(min, TIDECALC_INTERVAL_MINS);
	starttime = date_to_jtime(Y, M, D, H, min);
	// end is 8 days later
	endtime = starttime + TIDECALC_DAYS * 24 * 60;
	debugf(1,"initialiseTideTimes(%s to %s)\n",jctime(starttime),jctime(endtime));
}


int
TideCalcStation::calculateTideTimes()
{
	char cmd[FILENAME_MAX];
	char line[256];
	int Y0, M0, D0, h0; double m0;
	int Y1, M1, D1, h1; double m1;

	jtime_to_date(starttime, &Y0, &M0, &D0, &h0, &m0);
	jtime_to_date(endtime,   &Y1, &M1, &D1, &h1, &m1);

	// -em event mask, use pSsMm to suppress sun and moon info
	// -df date format, like strftime, default %Y-%m-%d
	// -tf time format, like strftime, default %l:%M %p %Z
	// -b and -e, dates YYYY-MM-DD HH:MM
	// -l location
	// -m mode, r=raw, p=plain, l=list stations
	// -f format, c=csv
	// -s hh:mm, step interval between each prediction
	// -z y UTC times, y or n
	// -u m Meters
	// setenv HFILE_PATH /path/to/harmonics.tcd
	// XXX turn off disclaimer
	// XXX save stderr to a proper location inside appdir
	sprintf(cmd, "./tide -l \"%s\" -m r -f c -s 00:%02d -z y -u m "
		"-b \"%d-%02d-%02d %02d:%02.0f\" "
		"-e \"%d-%02d-%02d %02d:%02.0f\" "
		" 2>tide.log ",
		(const char*)station, TIDECALC_INTERVAL_MINS,
		Y0, M0, D0, h0, m0,
		Y1, M1, D1, h1, m1);

	debugf(1,"calculateTideTimes using %s\n",cmd);
	FILE *pfp = popen(cmd, "r");
	int ii = 0;
	while (fgets(line, 255, pfp))
	{
		float level;
		time_t tim;
		int rc;
		rc = sscanf(line, "%*[^,],%ld,%f", &tim, &level);
		if (rc==2)
		{
			double j = time_t_2_jtime(tim);
			debugf(2,"  height[%3d]=%5.2f at %s\n", ii, level, jctime(j));
			height[ii++] = level;
		}
	}
	pclose(pfp);

	return(0);
}


int
TideCalcStation::calculateHighWaterTimes()
{
	char cmd[FILENAME_MAX];
	char line[256];
	int Y0, M0, D0, h0; double m0;
	int Y1, M1, D1, h1; double m1;

	jtime_to_date(starttime, &Y0, &M0, &D0, &h0, &m0);
	jtime_to_date(endtime,   &Y1, &M1, &D1, &h1, &m1);

	// -em event mask, use pSsMm to suppress sun and moon info
	// -df date format, like strftime, default %Y-%m-%d
	// -tf time format, like strftime, default %l:%M %p %Z
	// -b and -e, dates YYYY-MM-DD HH:MM
	// -l location
	// -m mode, r=raw, p=plain, l=list stations
	// -f format, c=csv
	// -s hh:mm, step interval between each prediction
	// -z y UTC times, y or n
	// -u m Meters
	// setenv HFILE_PATH /path/to/harmonics.tcd
	// XXX turn off disclaimer
	// XXX save stderr to a proper location inside appdir
	sprintf(cmd, "./tide -l \"%s\" -m p -f c -z y -u m -em pSsMm "
		"-df \"%%Y %%m %%d\" -tf \"%%H %%M\" "
		"-b \"%d-%02d-%02d %02d:%02.0f\" "
		"-e \"%d-%02d-%02d %02d:%02.0f\" "
		" 2>tide.log ",
		(const char*)station,
		Y0, M0, D0, h0, m0,
		Y1, M1, D1, h1, m1);

	debugf(1,"calculateHighWaterTimes using %s\n",cmd);
	FILE *pfp = popen(cmd, "r");
	int ii = 0;
	while (fgets(line, 255, pfp))
	{
		if (!strstr(line, "High Tide"))
			continue;
		// eg. Leith| Scotland,2010 05 13,01 48,5.12 m,High Tide
		int Y, M, D, h;
		float min, level;
		int rc;
		rc = sscanf(line, "%*[^,],%d %d %d,%d %f,%f", &Y, &M, &D, &h, &min, &level);
		if (rc==6)
		{
			double j = date_to_jtime(Y, M, D, h, min);
			debugf(2,"  HW[%3d]=%5.2f at %s (%f)\n", ii, level, jctime(j), j);
			jtime_hw[ii++] = j;
		}
	}
	jtime_hw[ii] = 0.0;
	pclose(pfp);

	return(0);
}


#define SIXHOURS (60*8) // a bit more to ensure a tide change is included


int
TideCalcStation::findTide(double jtime, float *tideheight)
{
	if (jtime < starttime+SIXHOURS || jtime > endtime-SIXHOURS)
	{
		initialiseTideTimes(jtime-SIXHOURS);
		calculateTideTimes();
		calculateHighWaterTimes();
	}
	int ii;
	ii = NINT((jtime - starttime)/TIDECALC_INTERVAL_MINS);
	if (ii<0 || ii>=TIDECALC_POINTS)
	{
		fprintf(stderr, "ERROR *** findTide called with out of range time ***\n");
		return(-1);
	}
	debugf(1,"findTide at %s is index %d height %f\n",jctime(jtime),ii,height[ii]);
	*tideheight = height[ii];
	return(0);
}


int
TideCalcStation::findNearestHighWater(double jtime, float *tideheight, double *jtimeHW)
{
	debugf(1,"findNearestHighWater(%s)\n",jctime(jtime));
	if (jtime < starttime+SIXHOURS || jtime > endtime-SIXHOURS)
	{
		initialiseTideTimes(jtime-SIXHOURS);
		calculateTideTimes();
		calculateHighWaterTimes();
	}

	int ii;
	double prevdiff = fabs(jtime - jtime_hw[0]), diff;
	for (ii=1; ii<TIDECALC_HW_POINTS; ii++)
	{
		diff = fabs(jtime - jtime_hw[ii]);
		if (diff > prevdiff)
		{
			--ii;
			break;
		}
		prevdiff = diff;
	}
	*tideheight = 0.0; // this version doesn't store height
	*jtimeHW = jtime_hw[ii];
	debugf(1, "  HW [%d] at %s is closest to %s (%f,%f-%f)\n", ii, jctime(*jtimeHW), jctime(jtime), *jtimeHW, jtime, fabs(*jtimeHW-jtime)/60.0);

	return(0);
}


/* --------------------------------------------------------------------------
 * TideCalc is an object which will return the tide height at a specified
 *   location and time. It will also return the nearest time when the tide
 * is at High Water. Any location and time can be given and used to call
 * the external "tide" program but return values are cached for speed.
 * If harmonics param to constructor is not null then it must be just the
 * filename without path as it is assumed to be located in the app path.
 */
TideCalc::TideCalc(const QString &harmonics)
{
	harmonicsFilename = harmonics.isEmpty() ? QString(DEFAULT_HARMONICS_FILE) : harmonics;

	QString appdir = qApp->applicationDirPath();
	harmonicsFilename = appdir + DIRSEPSTR + harmonicsFilename;
	loadTideDatabase(harmonicsFilename);

#if 0
	// No longer need LD_LIBRARY_PATH as all apps built with static libtcd
	static char ld[FILENAME_MAX];
	sprintf(ld, "LD_LIBRARY_PATH=%s", (const char*)appdir);
	putenv(ld);
#endif
}


TideCalc::~TideCalc()
{
}


/*
 * Load a different tide database (.tcd file)
 * Must be the full path to the file.
 * Must only be one filename not a directory or colon-separated list of files
 * (even though HFILE_PATH can accept that).
 */
bool
TideCalc::loadTideDatabase(const QString &filename)
{
	// Keep a copy of the database path
	harmonicsFilename = filename;

	/* Set some environment variables used by the "tide" program */
	/* It needs a set of harmonics so set HFILE_PATH */
	/* Must be static since putenv does not copy strings only pointers */
	/* Not sure what happens when this gets called two or more times */
	static char harm[FILENAME_MAX];
	sprintf(harm, "HFILE_PATH=%s", (const char*)harmonicsFilename);
	putenv(harm);

	/*
	 * Load the tide database now so we can check station names later
	 * XXX look in different directories if the load fails?
	 */
	return tidedatabase.load(harmonicsFilename);
}


/*
 * NOTE: returned location is +/-180 positive East.
 */
bool
TideCalc::getStationLocation(const QString &station, double *lat, double *lon)
{
	// XXX Check that database has been loaded first?
	if (!tidedatabase.setStation((const char*)station))
		return false;
	return tidedatabase.getStationLocation(lat, lon);
}


int
TideCalc::findTide(const QString &station, double jtime, float *tideheight)
{
	TideCalcStation *tcsp;

	// Find station in dictionary if present
	tcsp = stationdict.find(station);

	// If not present then create one and add it to the dictionary
	if (tcsp == 0)
	{
		tcsp = new TideCalcStation(station, jtime);
		stationdict.insert(station, tcsp);
	}

	return tcsp->findTide(jtime, tideheight);
}


int
TideCalc::findNearestHighWater(const QString &station, double jtime, float *tideheight, double *jtimeHW)
{
	TideCalcStation *tcsp;

	// Find station in dictionary if present
	tcsp = stationdict.find(station);

	// If not present then create one and add it to the dictionary
	if (tcsp == 0)
	{
		tcsp = new TideCalcStation(station, jtime);
		stationdict.insert(station, tcsp);
	}

	return tcsp->findNearestHighWater(jtime, tideheight, jtimeHW);
}


/* --------------------------------------------------------------------------
 * Moon
 * daysSinceNew - Calculate the number of days since the last New Moon
 * fractionFromSpringToNeap - Calculate the fraction (0..1) of the given time
 *   from the previous New Moon to the next Full Moon, ie. from Spring to Neap
 *   so 0.5 is half way from spring to neap.
 */
MoonCalc::MoonCalc()
{
	lastjtime = lastage = 0.0;
}


/*
 * Return the age, ie. fractional days since the last New Moon
 */
float
MoonCalc::daysSinceNew(double jtime)
{
	int lunation_number;
	double JD;

	// As long as the caller doesn't keep calling with the time
	// changing by less than a minute each time our cache is fine.
	if (fabs(jtime - lastjtime) < 1.0)
	{
		debugf(1, "MoonCalc::daysSinceNew(%s) %f (fast)\n", jctime(jtime), lastage);
		return lastage;
	}

	JD = jtime_to_JD(jtime);
	lastage = moon_days_since_new(JD, &lunation_number);
	lastjtime = jtime;
		debugf(1, "MoonCalc::daysSinceNew(%s) %f (slow)\n", jctime(jtime), lastage);
	return lastage;
}


double
MoonCalc::minsSinceNew(double jtime)
{
	// Assuming we are happy with the 24hour day
	return daysSinceNew(jtime) * 24 * 60;
}


double
MoonCalc::fractionFromSpringToNeap(double jtime)
{
	//static const double halfTime = SYNODIC_MONTH_IN_DAYS / 2.0;
	static const double quarterTime = SYNODIC_MONTH_IN_DAYS / 4.0;
#if 0
	double days_from_new = daysSinceNew(jtime);
	double days_from_spring = fmod(days_from_new, quarterTime);
	debugf(1,"Moon %s  days=%f from New, frac=%f from Spring\n",jctime(jtime),days_from_new,days_from_spring/quarterTime);
	return days_from_spring/quarterTime;
#endif
	double daysFromNewMoon = daysSinceNew(jtime);
	double daysFraction = daysFromNewMoon / SYNODIC_MONTH_IN_DAYS;
	double daysFromQuarter = fmod(daysFromNewMoon, quarterTime);
	//debugf(1, "daysfromnew %f, fracOfMonth %f, daysfromQ %f, fracFromStoN %f / %f\n",daysFromNewMoon,daysFraction,daysFromQuarter, (daysFromQuarter/quarterTime), 1-(daysFromQuarter/quarterTime));
	if (daysFraction > 0.75) return (1.0-(daysFromQuarter/quarterTime));
	else if (daysFraction > 0.5) return (daysFromQuarter/quarterTime);
	else if (daysFraction > 0.25) return (1.0-(daysFromQuarter/quarterTime));
	else return (daysFromQuarter/quarterTime);
}


/* --------------------------------------------------------------------------
 */
#ifdef MAIN
int
main(int argc, char *argv[])
{
	// Required application
	QApplication app(argc, argv, false);
	// Input
	char *station;
	double jtime, offset;
	// Tide predictor:
	TideCalc tidecalc;
	// Output:
	float tideheight;
	double jtimeHW;

	if (argc!=2) { printf("usage: tidecalc offset_in_mins\n"); exit(1); }

	station = "ABERDEEN";
	jtime = jtime_now(); jtime = date_to_jtime(2010,5,5,12,0.0);
	offset = atol(argv[1]);

	tidecalc.findNearestHighWater(station, jtime, &tideheight, &jtimeHW);
	tidecalc.findNearestHighWater(station, jtime+offset, &tideheight, &jtimeHW);

	MoonCalc mooncalc;
	printf("Moon age %f at %s\n", mooncalc.daysSinceNew(jtime-mooncalc.minsSinceNew(jtime)), jctime(jtime-mooncalc.minsSinceNew(jtime)));
	printf("Moon age %f at %s\n", mooncalc.daysSinceNew(jtime), jctime(jtime));
	printf("Moon age %f at %s\n", mooncalc.daysSinceNew(jtime+offset), jctime(jtime+offset));

	for (int ii=0; ii<30; ii++)
		printf("%2d Since Spring %f at %s\n", ii, mooncalc.fractionFromSpringToNeap(jtime+1440*ii), jctime(jtime+1440*ii));

	return(0);
}
#endif
