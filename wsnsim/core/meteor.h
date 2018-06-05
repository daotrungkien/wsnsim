#pragma once

#include <math.h>

#include "wsnsim.h"

extern "C" {
#include "../lib/spa/spa.h"
}

namespace wsn {

class sun : public wsn_type {
public:

	// Reda, Ibrahim, and Afshin Andreas. "Solar position algorithm for solar radiation applications." Solar energy 76.5 (2004): 577-589.
	static void get_position(double lat, double lon, double timezone, std::chrono::system_clock::time_point t, double &elevation, double& azimuth) {
		time_t tt = std::chrono::system_clock::to_time_t(t);

		tm tinf;
		localtime_s(&tinf, &tt);
		tinf.tm_year += 1900;
		tinf.tm_mon++;
		tinf.tm_yday++;


		spa_data spa;
		spa.year = tinf.tm_year;
		spa.month = tinf.tm_mon;
		spa.day = tinf.tm_mday;
		spa.hour = tinf.tm_hour;
		spa.minute = tinf.tm_min;
		spa.second = tinf.tm_sec;
		spa.timezone = timezone;
		spa.delta_ut1 = 0;
		spa.delta_t = 0;
		spa.latitude = lat;
		spa.longitude = lon;
		spa.elevation = 0;
		spa.pressure = 760;
		spa.temperature = 25;
		spa.slope = 30;
		spa.azm_rotation = -10;
		spa.atmos_refract = 0.5667;
		spa.function = SPA_ZA;

		int result = spa_calculate(&spa);
		assert(result == 0);

		elevation = (90 - spa.zenith)*M_PI / 180;
		azimuth = spa.azimuth*M_PI / 180;
	}



	/* https://stackoverflow.com/questions/8708048/position-of-the-sun-given-time-of-day-latitude-and-longitude
	- Michalsky, J.J. 1988. The Astronomical Almanac's algorithm for approximate solar position (1950-2050). Solar Energy. 40(3):227-235.
	- Michalsky, J.J. 1989. Errata.Solar Energy. 43(5) : 323.
	- Spencer, J.W. 1989. Comments on "The Astronomical Almanac's Algorithm for Approximate Solar Position (1950-2050)".Solar Energy. 42(4) : 353.
	- Walraven, R. 1978. Calculating the position of the sun.Solar Energy. 20 : 393 - 397. */
	static void get_position2(double lat, double lon, double timezone, std::chrono::system_clock::time_point t, double &elevation, double& azimuth) {
		constexpr double deg2rad = M_PI / 180.;

		time_t tt = std::chrono::system_clock::to_time_t(t);

		tm tinf;
		localtime_s(&tinf, &tt);
		tinf.tm_year += 1900;
		tinf.tm_mon++;
		tinf.tm_yday++;

		if (tinf.tm_mon < 3) {
			tinf.tm_mon += 12;
			tinf.tm_yday--;
		}

		// Get day of the year
		int day = tinf.tm_yday;

		// Get Julian date - 2400000
		double hour = (tinf.tm_hour + timezone) + tinf.tm_min / 60. + tinf.tm_sec / 3600.; // hour plus fraction
		int delta = tinf.tm_year - 1949;
		int leap = delta / 4;
		double jd = 32916.5 + (delta * 365 + leap + day) + hour / 24.;

		// The input to the Atronomer's almanach is the difference between
		// the Julian date and JD 2451545.0 (noon, 1 January 2000)
		double time = jd - 51545.;

		// Ecliptic coordinates
		// 51545.0 + 2.4e6 = noon 1 jan 2000

		// Mean longitude
		double mnlong = 280.460 + .9856474 * time;
		mnlong = fmod(mnlong, 360.);
		if (mnlong < 0) mnlong += 360.;

		// Mean anomaly
		double mnanom = 357.528 + .9856003 * time;
		mnanom = fmod(mnanom, 360.);
		if (mnanom < 0) mnanom += 360.;
		mnanom *= deg2rad;

		// Ecliptic longitude and obliquity of ecliptic
		double eclong = mnlong + 1.915 * sin(mnanom) + 0.020 * sin(2. * mnanom);
		eclong = fmod(eclong, 360.);
		if (eclong < 0) eclong += 360.;
		eclong *= deg2rad;

		double oblqec = 23.439 - 0.0000004 * time;
		oblqec *= deg2rad;

		// Celestial coordinates

		// Right ascension and declination
		double num = cos(oblqec) * sin(eclong),
			den = cos(eclong),
			ra = atan2(num, den);
		if (den < 0.) ra += M_PI;
		else if (num <= 0.) ra += 2 * M_PI;
		
		double dec = asin(sin(oblqec) * sin(eclong));

		// Local coordinates

		// Greenwich mean sidereal time in hours
		double gmst = 6.697375 + .0657098242 * time + hour;
		gmst = fmod(gmst, 24.);
		if (gmst < 0.) gmst += 24.;

		// Local mean sidereal time
		double lmst = gmst + lon / 15.;
		lmst = fmod(lmst, 24.);
		if (lmst < 0.) lmst += 24.;
		lmst *= 15. * M_PI / 180;

		// Hour angle
		double ha = lmst - ra;
		if (ha < -M_PI) ha += 2 * M_PI;
		if (ha > M_PI) ha -= 2 * M_PI;

		// Latitude to radians
		lat *= M_PI/180;

		// Solar azimuth and elevation
		elevation = asin(sin(dec) * sin(lat) + cos(dec) * cos(lat) * cos(ha));
		azimuth = asin(-cos(dec) * sin(ha) / cos(elevation));

		// For logic and names, see Spencer, J.W. 1989. Solar Energy. 42(4) : 353
		bool cosAzPos = (0 <= sin(dec) - sin(elevation) * sin(lat)),
			sinAzNeg = (sin(azimuth) < 0);
		if (cosAzPos && sinAzNeg) azimuth += 2 * M_PI;
		if (!cosAzPos) azimuth = M_PI - azimuth;
	}

	static double get_radiation_power(double lat, double lon, double timezone, std::chrono::system_clock::time_point t) {
		double azimuth, elevation;
		sun::get_position(lat, lon, timezone, t, elevation, azimuth);

		return elevation < 0 ? 0 : 1367.*sin(elevation);
	}
};


}