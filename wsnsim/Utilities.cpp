//#include "stdafx.h"
#include "Utilities.h"





std::string format_time(time_t t, const char* format) {
	tm ltime;
	localtime_s(&ltime, &t);

	char timestr[50];
	strftime(timestr, sizeof(timestr)/sizeof(timestr[0]), format, &ltime);
	return timestr;
}

std::wstring format_time(time_t t, const wchar_t* format) {
	tm ltime;
	localtime_s(&ltime, &t);

	wchar_t timestr[50];
	wcsftime(timestr, sizeof(timestr)/sizeof(timestr[0]), format, &ltime);
	return timestr;
}
