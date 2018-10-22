/* > tidedata.cpp
 * 1.04 arb Sun Jul 11 13:45:25 BST 2010 - Rewrite parser
 * 1.03 arb Fri Jun 18 04:54:17 BST 2010 - interpolate between neap and spring
 * 1.02 arb Thu Jun 17 00:23:17 BST 2010 - interpolate via x,y
 * 1.01 arb Mon Jun 14 22:06:39 BST 2010 - interpolate stream info using cspline
 * 1.00 arb Thu May 27 16:38:49 BST 2010
 */

static const char SCCSid[] = "@(#)tidedata.cpp  1.04 (C) 2010 arb Load tidal data";


#include <stdio.h>
#include <math.h>
#include <qstringlist.h>
#include "satlib/dundee.h" // for cspline and splint
#include "tidedata.h"


#define ISIGN(v) ((v) >= 0 ? 1 : -1)


/* ----------------------------------------------------------------------------
 * TidalLevel
 * Read from files with names like *T10.T1
 * chart    N  place lat min lon min MHWS MHWN MLWN MLWS datum remarks
 * BA1481 236 DUNDEE  56 27  -2 58   5.4  4.3  1.9  0.7  2.9   BELOW ORDNANCE DATUM    NEWLYN  DUSTIN       KT
 * sometimes you get datum and remarks being three 9999 then two DUSTIN
 */
TidalLevel::TidalLevel(const QString &line)
{
	int num;
	int adeg, amin, odeg, omin;

	ok = false;
	lat = lon = 0;
	mhws = mhwn = mlwn = mlws = 0;
	current_level = 0;

	QStringList fields = QStringList::split('\t', line, true);
	if (fields.count() < 11)
		return;
	chart = fields[0];
	num = fields[1].toInt();
	name = fields[2];
	adeg = fields[3].toInt();
	amin = fields[4].toInt();
	odeg = fields[5].toInt();
	omin = fields[6].toInt();
	mhws = fields[7].toDouble();
	mhwn = fields[8].toDouble();
	mlwn = fields[9].toDouble();
	mlws = fields[10].toDouble();
	lat = (float)adeg + (float)amin / 60.0 * ISIGN(adeg);
	lon = (float)odeg + (float)omin / 60.0 * ISIGN(odeg);
	ok = true;
}


/* ----------------------------------------------------------------------------
 * TidalStreams
 * Read from files with names like *C10.C1
 * chart  HW RefPort  Num Diamond lat min  lon min  13x[bearing SpringKnots NeapKnots 3x9999] DUSTIN CLEMENTS
 * BA1481 HW ABERDEEN 244 A       56 25.89 -2 36.99 59	1.2	0.6	9999	9999	9999	63	0.6	0.3	9999	9999	9999	112	0.3	0.1	9999	9999	9999	195	0.6	0.3	9999	9999	9999	213	1.1	0.5	9999	9999	9999	223	1.1	0.5	9999	9999	9999	231	0.9	0.4	9999	9999	9999	240	0.6	0.3	9999	9999	9999	268	0.3	0.1	9999	9999	9999	334	0.4	0.2	9999	9999	9999	23	0.8	0.4	9999	9999	9999	34	1.1	0.5	9999	9999	9999	52	1.2	0.6	9999	9999	9999	DUSTIN	CLEMENTS
 */

const float TidalStream::hour[] = {0,1,2,3,4,5,6,7,8,9,10,11,12};
const float TidalStream::cspline_natural = 1.0e30;

TidalStream::TidalStream(const QString &line)
{
	int ii;
	int num;
	int adeg, odeg;
	float amin, omin;

	ok = false;
	lat = lon = 0;
	for (ii=0; ii<TIDALSTREAM_NUMRATES; ii++)
	{
		bearing[ii] = 0;
		springRate[ii] = neapRate[ii] = 0;
	}
	current_bearing = 0;
	current_rate = 0;

	QStringList fields = QStringList::split('\t', line, true);
	if (fields.count() < 9 + 3*TIDALSTREAM_NUMRATES)
		return;

	chart = fields[0];
	refHW = (fields[1].left(1) == 'H'); // "HW", or "LW" for LE HAVRE
	ref   = fields[2];
	num   = fields[3].toInt();
	name  = fields[4];
	adeg  = fields[5].toInt();
	amin  = fields[6].toDouble();
	odeg  = fields[7].toInt();
	omin  = fields[8].toDouble();
	lat = (float)adeg + amin / 60.0 * ISIGN(adeg);
	lon = (float)odeg + omin / 60.0 * ISIGN(odeg);
	for (ii=0; ii<TIDALSTREAM_NUMRATES; ii++)
	{
		bearing[ii]    = fields[9 + ii*6 + 0].toDouble();
		springRate[ii] = fields[9 + ii*6 + 1].toDouble();
		neapRate[ii]   = fields[9 + ii*6 + 2].toDouble();
	}

	// Calculate interpolation coefficients
	cspline(hour, bearing,    TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, bearing_coeff);
	cspline(hour, springRate, TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, spring_coeff);
	cspline(hour, neapRate,   TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, neap_coeff);
	for (ii=0; ii<TIDALSTREAM_NUMRATES; ii++)
	{
		xspring[ii] = sin(RAD(bearing[ii])) * springRate[ii];
		xneap[ii]   = sin(RAD(bearing[ii])) * neapRate[ii];
		yspring[ii] = cos(RAD(bearing[ii])) * springRate[ii];
		yneap[ii]   = cos(RAD(bearing[ii])) * neapRate[ii];
	}
	cspline(hour, xspring, TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, xspring_coeff);
	cspline(hour, yspring, TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, yspring_coeff);
	cspline(hour, xneap,   TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, xneap_coeff);
	cspline(hour, yneap,   TIDALSTREAM_NUMRATES, cspline_natural, cspline_natural, yneap_coeff);

	ok = true;
}


/*
 * Given a time difference from HW at the reference station
 * return the bearing of the stream and its rate at spring/neap tide.
 * The value is interpolated from the table unless the time is exactly
 * on the hour. Range of mins is -6 hours to +6 hours.
 */
bool
TidalStream::getStreamMinsFromRef(double mins, float *pbearing, float *pspringRate, float *pneapRate)
{
	debugf(1, "TidalStream %s query %f hours from HW\n", (const char*)name, mins/60.0);
	if (mins < -6 * 60 || mins > 6 * 60)
	{
		// XXX
		if (mins < -7 * 60 || mins > 7 * 60) { fprintf(stderr, "WARNING asking for tide at offset %f hrs\n",mins/60.0); *pbearing=0; *pspringRate=0; *pneapRate=0; return false; }
		if (mins < -6 * 60) mins = -6*60;
		if (mins >  6 * 60) mins =  6*60; // XXX extrapolate don't clip
	}
	// Offset -6 hours should be zero.
	mins += 6 * 60;
	int index = (int)(mins / 60.0);
	// See if we can return without interpolation
	if (fmod(mins, 60) == 0)
	{
		*pbearing = bearing[index];
		*pspringRate = springRate[index];
		*pneapRate = neapRate[index];
		return true;
	}
	// Interpolate between this one and the next one
#if 0
	*pbearing    = splint(hour, bearing,    bearing_coeff, TIDALSTREAM_NUMRATES, 0, mins/60.0);
	*pspringRate = splint(hour, springRate, spring_coeff,  TIDALSTREAM_NUMRATES, 0, mins/60.0);
	*pneapRate   = splint(hour, neapRate,   neap_coeff,    TIDALSTREAM_NUMRATES, 0, mins/60.0);
#endif
	float xspringval, xneapval, yspringval, yneapval;
	xspringval = splint(hour, xspring,  xspring_coeff, TIDALSTREAM_NUMRATES, 0, mins/60.0);
	yspringval = splint(hour, yspring,  yspring_coeff, TIDALSTREAM_NUMRATES, 0, mins/60.0);
	xneapval   = splint(hour, xneap,    xneap_coeff,   TIDALSTREAM_NUMRATES, 0, mins/60.0);
	yneapval   = splint(hour, yneap,    yneap_coeff,   TIDALSTREAM_NUMRATES, 0, mins/60.0);
	// XXX calculate bearing using xspring/yspring OR xneap/yneap ???
	// XXX need to interpolate as the given time will be between spring and neap tide!!!
	*pbearing = DEG(atan2(xspringval, yspringval));
	*pspringRate = sqrt(xspringval*xspringval + yspringval*yspringval);
	*pneapRate   = sqrt(xneapval*xneapval + yneapval*yneapval);
	debugf(1, "  mins %f index %d bearing %f rate %f/%f\n", mins, index, *pbearing, *pspringRate, *pneapRate);
	return true;
}


bool
TidalStream::getStreamMinsFromRefAndMoon(double minsfromref, double fractionFromSpring, float *bearing, float *rate)
{
	float springrate, neaprate;
	bool rc;

	rc = getStreamMinsFromRef(minsfromref, &current_bearing, &springrate, &neaprate);

	// Now interpolate between spring and neap
	// Could use linear interpolation rate=(spring+neap)/2
	// But assume it's more like a cosine from spring to neap
	current_rate = neaprate + (springrate - neaprate) * cos(PI/2.0 * (fractionFromSpring));

	if (bearing)
		*bearing = current_bearing;
	if (rate)
		*rate = current_rate;
	debugf(1, "  rate %f at phase fraction %f\n", current_rate, fractionFromSpring);

	return rc;
}


/* ----------------------------------------------------------------------------
 * eg.
grep -h '	D	56	2' tidedata/*10.C1 | sort -u | ./td
 */
#ifdef MAIN
int main(int argc, char *argv[])
{
	char line[2048];
	double jtime_wanted = 58034364.0;
	double jtime_ref_hw = 58034623.0;
	float bearing, rate;

	while (fgets(line, 2048, stdin))
	{
		TidalStream ts(line);
		if (!ts.isOk()) { printf("ERROR %s",line); continue; }
		ts.getStreamMinsFromRefAndMoon(jtime_wanted - jtime_ref_hw,
			0.860775, &bearing, &rate);
		printf("bearing %f rate %f\n", bearing, rate);
	}
	return(0);
}
#endif
