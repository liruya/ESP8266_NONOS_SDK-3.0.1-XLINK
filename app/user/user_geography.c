#include "user_geography.h"

#define	PI				3.14159265
#define	PRECISION		0.1
#define	UTO_DEFAULT		180
#define	ARC				0.0174532925

const uint8_t month_days_normal[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const uint8_t month_days_leap[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const double H = 0.833;

LOCAL float mLongitude;
LOCAL float mLatitude;
LOCAL uint16_t mYear;
LOCAL uint8_t mMonth;
LOCAL uint8_t mDay;
LOCAL int16_t mZone;
LOCAL int32_t mSunrise;
LOCAL int32_t mSunset;

LOCAL uint8_t ESPFUNC get_zone(float longitude) {
	if(longitude > 172.5) {
		return 12;
	}
	if(longitude < -172.5) {
		return -12;
	}
	return (longitude+7.5)/15;
}

LOCAL bool ESPFUNC is_leap_year(uint16_t year) {
	if ( year%4 != 0 ) {
		return false;
	}
	if ( year%100 == 0 && year%400 != 0 ) {
		return false;
	}
	return true;
}

LOCAL bool ESPFUNC is_valid_location(float longitude, float latitude) {
	if(longitude < -180 || longitude > 180) {
		return false;
	}
	if(latitude < -60 || latitude > 60) {
		return false;
	}
	return true;
}

LOCAL bool ESPFUNC is_valid_zone(int16_t zone) {
	if(zone > 1200 || zone < -1200 || zone%100 >= 60 || zone%100 <= -60)
	{
		return false;
	}
	return true;
}

LOCAL uint32_t ESPFUNC get_days(int year, int month, int day) {
	if(year < 2000 || month < 1 || month > 12) {
		return 0;
	}
	uint8_t monthdays;
	if(is_leap_year(year)) {
		monthdays = month_days_leap[month-1];
	} else {
		monthdays = month_days_normal[month-1];
	}
	if(day < 1 || day > monthdays) {
		return 0;
	}

	uint32_t i;
	uint32_t sum = 0;
	for(i = 2000; i < year; i++) {
		if(is_leap_year(i)) {
			sum += 366;
		} else {
			sum += 365;
		}
	}
	if(is_leap_year(year)) {
		for(i = 0; i < month-1; i++) {
			sum += month_days_leap[i];
		}
	} else {
		for(i = 0; i < month-1; i++) {
			sum += month_days_normal[i];
		}
	}
	sum += day;
	return sum;
}

//求格林威治时间公元2000.1.1到计算日的世纪数
LOCAL double ESPFUNC get_centuries(int days, double uto) {
	return ((double)(days+uto/360))/36525;
}

//求太阳的平黄径
LOCAL double ESPFUNC get_l_sun(double centuries) {
	return (280.460+36000.770*centuries);
} 

//求太阳的平近点角
LOCAL double ESPFUNC get_g_sun(double centuries) {
	return (357.528+35999.050*centuries);
}

//求黄道经度
LOCAL double ESPFUNC get_ecliptic_longitude(double l_sun, double g_sun) {
	return (l_sun+1.915*sin(g_sun*PI/180)+0.02*sin(2*g_sun*PI/180));
}

//求地球视角
LOCAL double ESPFUNC get_earth_tilt(double centuries) {
	return (23.4393-0.0130*centuries);
}

//求太阳偏差
LOCAL double ESPFUNC get_sun_deviation(double earth_tilt, double ecliptic_longitude) {
	return (180/PI*asin(sin(PI/180*earth_tilt)*sin(PI/180*ecliptic_longitude)));
}

//求格林威治时间的太阳时间角GHA
LOCAL double ESPFUNC get_gha(double uto, double g_sun, double ecliptic_longitude) {
	return (uto - 180 - 1.915*sin(g_sun*PI/180) - 0.02*sin(2*g_sun*PI/180) + 2.466*sin(2*ecliptic_longitude*PI/180) - 0.053*sin(4*ecliptic_longitude*PI/180) );
}

//求修正值e
LOCAL double ESPFUNC get_e(double h, double glat, double sun_deviation) {
	return 180/PI*acos((sin(h*PI/180)-sin(glat*PI/180)*sin(sun_deviation*PI/180))/(cos(glat*PI/180)*cos(sun_deviation*PI/180)));
}

//求日出时间
LOCAL double ESPFUNC ut_sunrise(double uto, double gha, double glong, double e) {
	return (uto-(gha+glong+e));
}

//求日落时间
LOCAL double ESPFUNC ut_sunset(double uto, double gha, double glong, double e) {
	return (uto-(gha+glong-e));
}

// int32_t ESPFUNC get_sunrise(double glong, double glat, uint32_t year, uint8_t month, uint8_t day) {
// 	double uto = UTO_DEFAULT;
// 	double ut;
// 	double d;
// 	uint32_t days = get_days(year, month, day);
// 	do {
// 		double centuries = get_centuries(days, uto);
// 		double g_sun = get_g_sun(centuries);
// 		double l_sun = get_l_sun(centuries);
// 		double ecliptic_longitude = get_ecliptic_longitude(l_sun, g_sun);
// 		double gha = get_gha(uto, g_sun, ecliptic_longitude);
// 		double earth_tilt = get_earth_tilt(centuries);
// 		double sun_deviation = get_sun_deviation(earth_tilt, ecliptic_longitude);
// 		double e = get_e(H, glat, sun_deviation);
// 		ut = ut_sunrise(uto, gha, glong, e);
// 		if(ut >= uto) {
// 			d = ut - uto;
// 		} else {
// 			d = uto - ut;
// 		}
// 		uto = ut;
// 	} while(d >= PRECISION);
// 	return (ut/15+get_zone(glong))*60;
// }

// int32_t ESPFUNC get_sunset(double glong, double glat, uint32_t year, uint8_t month, uint8_t day) {
// 	double uto = UTO_DEFAULT;
// 	double ut;
// 	double d;
// 	uint32_t days = get_days(year, month, day);
// 	do {
// 		double centuries = get_centuries(days, uto);
// 		double g_sun = get_g_sun(centuries);
// 		double l_sun = get_l_sun(centuries);
// 		double ecliptic_longitude = get_ecliptic_longitude(l_sun, g_sun);
// 		double gha = get_gha(uto, g_sun, ecliptic_longitude);
// 		double earth_tilt = get_earth_tilt(centuries);
// 		double sun_deviation = get_sun_deviation(earth_tilt, ecliptic_longitude);
// 		double e = get_e(H, glat, sun_deviation);
// 		ut = ut_sunset(uto, gha, glong, e);
// 		if(ut >= uto) {
// 			d = ut - uto;
// 		} else {
// 			d = uto - ut;
// 		}
// 		uto = ut;
// 	} while(d >= PRECISION);
// 	return (ut/15+get_zone(glong))*60;
// }

bool ESPFUNC get_sunrise_sunset(float glong, float glat, uint16_t year, uint8_t month, uint8_t day, int16_t zone, uint16_t *sunrise, uint16_t *sunset){
	if(is_valid_location(glong, glat) == false) {
		return false;
	}
	if(year < 2000 || month < 1 || month > 12) {
		return false;
	}
	uint8_t monthdays;
	if(is_leap_year(year)) {
		monthdays = month_days_leap[month-1];
	} else {
		monthdays = month_days_normal[month-1];
	}
	if(day < 1 || day > monthdays) {
		return false;
	}
	if(is_valid_zone(zone) == false) {
		return false;
	}

	if(mLongitude == glong && mLatitude == glat && mYear == year && mMonth == month && mDay == day && mZone == zone) {
		*sunrise = mSunrise;
		*sunset = mSunset;
		// app_log("%d:%d - %d:%d", mSunrise/60, mSunrise%60, mSunset/60, mSunset%60);
		return true;
	}

	mLongitude = glong;
	mLatitude = glat;
	mYear = year;
	mMonth = month;
	mDay = day;
	mZone = zone;

	double ut;
	double d;
	uint32_t days = get_days(year, month, day);

	double uto = UTO_DEFAULT;
	do {
		double centuries = get_centuries(days, uto);
		double g_sun = get_g_sun(centuries);
		double l_sun = get_l_sun(centuries);
		double ecliptic_longitude = get_ecliptic_longitude(l_sun, g_sun);
		double gha = get_gha(uto, g_sun, ecliptic_longitude);
		double earth_tilt = get_earth_tilt(centuries);
		double sun_deviation = get_sun_deviation(earth_tilt, ecliptic_longitude);
		double e = get_e(H, glat, sun_deviation);
		ut = ut_sunrise(uto, gha, glong, e);
		if(ut >= uto) {
			d = ut - uto;
		} else {
			d = uto - ut;
		}
		uto = ut;
	} while(d >= PRECISION);
	mSunrise = ut*4+(zone/100)*60+(zone%60);
	if(mSunrise < 0) {
		mSunrise += 1440;
	} else if (mSunrise >= 1440) {
		mSunrise -= 1440;
	}
	// mSunrise = (ut/15+get_zone(glong))*60;

	uto = UTO_DEFAULT;	
	do {
		double centuries = get_centuries(days, uto);
		double g_sun = get_g_sun(centuries);
		double l_sun = get_l_sun(centuries);
		double ecliptic_longitude = get_ecliptic_longitude(l_sun, g_sun);
		double gha = get_gha(uto, g_sun, ecliptic_longitude);
		double earth_tilt = get_earth_tilt(centuries);
		double sun_deviation = get_sun_deviation(earth_tilt, ecliptic_longitude);
		double e = get_e(H, glat, sun_deviation);
		ut = ut_sunset(uto, gha, glong, e);
		if(ut >= uto) {
			d = ut - uto;
		} else {
			d = uto - ut;
		}
		uto = ut;
	} while(d >= PRECISION);
	mSunset = ut*4+(zone/100)*60+zone%60;
	if(mSunset < 0) {
		mSunset += 1440;
	} else if(mSunset >= 1440) {
		mSunset -= 1440;
	}
	// mSunset = (ut/15+get_zone(glong))*60;

	// app_log("%d:%d - %d:%d", mSunrise/60, mSunrise%60, mSunset/60, mSunset%60);
	*sunrise = mSunrise;
	*sunset = mSunset;
	return true;
}
