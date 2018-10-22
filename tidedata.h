#ifndef TIDEDATA_H
#define TIDEDATA_H


#include <qstring.h>


class TidalLevel
{
public:
	TidalLevel(const QString &line);
	bool    isOk()     const { return ok; }
	double  getLat()   const { return lat; }
	double  getLon()   const { return lon; }
	QString getChart() const { return chart; }
	QString getName()  const { return name; }
	float  getMHWS()   const { return mhws; }
	float  getMHWN()   const { return mhwn; }
	float  getMLWN()   const { return mlwn; }
	float  getMLWS()   const { return mlws; }
	// The current level is calculated externally by TideCalc
	// but this is a convenient place to store the result
	void   setCurrentLevel(float lv) { current_level = lv; }
	float  getCurrentLevel() const   { return current_level; }
private:
	bool ok;
	QString chart, name;
	double lat, lon;
	float mhws, mhwn, mlwn, mlws;
	float current_level;
};


#define TIDALSTREAM_NUMRATES 13   // 6 before HW, one at HW, 6 after HW

class TidalStream
{
public:
	TidalStream(const QString &line);
	bool isOk()        const { return ok; }
	double  getLat()   const { return lat; }
	double  getLon()   const { return lon; }
	QString getChart() const { return chart; }
	QString getName()  const { return name; }
	QString getRef()   const { return ref; }   // times are referenced to this place
	bool refAtHW()     const { return refHW; } // when the location is at High Water
	bool getStreamMinsFromRef(double minutes, float *bearing, float *springRate, float *neapRate);
	bool getStreamMinsFromRefAndMoon(double minsfromref, double fractionFromSpring, float *bearing, float *rate);
	float getCurrentBearing() const { return current_bearing; }
	float getCurrentRate()    const { return current_rate; }
private:
	bool ok;
	QString chart, name, ref;
	bool refHW;
	double lat, lon;
	float bearing[TIDALSTREAM_NUMRATES];
	float springRate[TIDALSTREAM_NUMRATES];
	float neapRate[TIDALSTREAM_NUMRATES];
	float bearing_coeff[TIDALSTREAM_NUMRATES];
	float spring_coeff[TIDALSTREAM_NUMRATES];
	float neap_coeff[TIDALSTREAM_NUMRATES];
	float xspring[TIDALSTREAM_NUMRATES];
	float xneap[TIDALSTREAM_NUMRATES];
	float yspring[TIDALSTREAM_NUMRATES];
	float yneap[TIDALSTREAM_NUMRATES];
	float xspring_coeff[TIDALSTREAM_NUMRATES];
	float xneap_coeff[TIDALSTREAM_NUMRATES];
	float yspring_coeff[TIDALSTREAM_NUMRATES];
	float yneap_coeff[TIDALSTREAM_NUMRATES];
	static const float hour[TIDALSTREAM_NUMRATES];
	static const float cspline_natural;
	float current_bearing;
	float current_rate;
};


#endif // TIDEDATA_H
