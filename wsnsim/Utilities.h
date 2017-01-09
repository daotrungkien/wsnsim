#pragma once


#include <winsock2.h>
#include <ws2tcpip.h>
#include <tchar.h>
#include <time.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <vector>
#include <list>
#include <stack>
#include <functional>



typedef std::basic_string<TCHAR> tstring;
typedef std::basic_istringstream<TCHAR> tistringstream;
typedef std::basic_ostringstream<TCHAR> tostringstream;
typedef std::basic_stringstream<TCHAR> tstringstream;
typedef std::basic_ifstream<TCHAR> tifstream;
typedef std::basic_ofstream<TCHAR> tofstream;
typedef std::basic_fstream<TCHAR> tfstream;


class finally {
protected:
	std::function<void()> m_finalizer;
#if _MSC_VER >= 1800	// MSVC 2013+
	finally() = delete;
#endif

public:
#if _MSC_VER >= 1800	// MSVC 2013+
	finally( const finally& other ) = delete;
#endif

	finally( std::function<void()> finalizer )
		: m_finalizer(finalizer)
	{}

	~finally() {
		if (m_finalizer) m_finalizer();
	}
};



#if defined(UNICODE) || defined(_UNICODE)
	#define tcout std::wcout
	#define tcin std::wcin
#else
	#define tcout std::cout
	#define tcin std::cin
#endif



template <class T, class num_type>
typename std::enable_if<std::is_same<T, char>::value, std::basic_string<T>>::type
to_basic_string(num_type n) {
    return std::to_string(n);
}

template <class T, class num_type>
typename std::enable_if<std::is_same<T, wchar_t>::value, std::basic_string<T>>::type
to_basic_string(num_type n) {
    return std::to_wstring(n);
}

template <class num_type>
tstring to_tstring(num_type n) {
	return to_basic_string<TCHAR, num_type>(n);
}






template <class T1, class T2>
typename std::enable_if<std::is_same<T1, char>::value, std::basic_string<T1>>::type 
as_basic_string(const std::basic_string<T2>& s) {
	return std::basic_string<T1>(s.begin(), s.end());
}

template <class T1, class T2>
typename std::enable_if<std::is_same<T1, wchar_t>::value, std::basic_string<T1>>::type 
as_basic_string(const std::basic_string<T2>& s) {
	return std::basic_string<T1>(s.begin(), s.end());
}


template <class T> std::string as_string(const std::basic_string<T>& s) {
	return as_basic_string<char, T>(s);
}

template <class T> std::wstring as_wstring(const std::basic_string<T>& s) {
	return as_basic_string<wchar_t, T>(s);
}

template <class T> tstring as_tstring(const std::basic_string<T>& s) {
	return as_basic_string<TCHAR, T>(s);
}





template <class T> std::basic_string<T> strupper(const std::basic_string<T>& s) {
	std::basic_string<T> s1(s);
	std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
	return s1;
}

template <class T> std::basic_string<T> strlower(const std::basic_string<T>& s) {
	std::basic_string<T> s1(s);
	std::transform(s1.begin(), s1.end(), s1.begin(), ::tolower);
	return s1;
}




template <class T> std::basic_string<T> ltrim(const std::basic_string<T>& sz) {
	std::basic_string<T> s(sz);
	s.erase(s.begin(), find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(isspace))));
	return s;
}

template <class T> std::basic_string<T> rtrim(const std::basic_string<T>& sz) {
	std::basic_string<T> s(sz);
	s.erase(find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(isspace))).base(), s.end());
	return s;
}

template <class T> std::basic_string<T> trim(const std::basic_string<T>& sz) {
	return ltrim(rtrim(sz));
}





template <class T>
std::vector< typename std::enable_if<std::is_same<T, char>::value, std::basic_string<T>>::type >
split(const std::basic_string<T>& s, const char* delims = " \t") {
    std::vector<std::basic_string<T>> result;
    std::basic_string<T>::size_type pos = 0;
    while (std::basic_string<T>::npos != (pos = s.find_first_not_of(delims, pos))) {
        auto pos2 = s.find_first_of(delims, pos);
        result.emplace_back(s.substr(pos, std::basic_string<T>::npos == pos2 ? pos2 : pos2 - pos));
        pos = pos2;
    }
    return result;
}

template <class T>
std::vector< typename std::enable_if<std::is_same<T, wchar_t>::value, std::basic_string<T>>::type >
split(const std::basic_string<T>& s, const wchar_t* delims = L" \t") {
    std::vector<std::basic_string<T>> result;
    std::basic_string<T>::size_type pos = 0;
    while (std::basic_string<T>::npos != (pos = s.find_first_not_of(delims, pos))) {
        auto pos2 = s.find_first_of(delims, pos);
        result.emplace_back(s.substr(pos, std::basic_string<T>::npos == pos2 ? pos2 : pos2 - pos));
        pos = pos2;
    }
    return result;
}





template <class T> std::basic_string<T> formatstr(const T* szFmt, ...) {
	va_list args;
	va_start(args, szFmt);
	std::basic_string<T> msgstr = formatstr(szFmt, args);
	va_end(args);
	return msgstr;
}


template <class T> std::basic_string<T> formatstr(const T* szFmt, va_list args) {
#ifdef _MFC_VER
	CStringT<T, StrTraitMFC<T>> msg;
	msg.FormatV(szFmt, args);
	return std::basic_string<T>(msg);

#else
	// Ref: http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf#69911
	// Allocate a buffer on the stack that's big enough for us almost
	// all the time.  Be prepared to allocate dynamically if it doesn't fit.
	size_t size = 1024;
	TCHAR stackbuf[1024];
	std::vector<TCHAR> dynamicbuf;
	TCHAR* buf = &stackbuf[0];
	va_list ap_copy;

	while (1) {
		// Try to vsnprintf into our buffer.
		va_copy(ap_copy, args);
		int needed = _vsntprintf_s(buf, size, size, szFmt, args);
		va_end(ap_copy);

		// NB. C99 (which modern Linux and OS X follow) says vsnprintf
		// failure returns the length it would have needed.  But older
		// glibc and current Windows return -1 for failure, i.e., not
		// telling us how much was needed.

		if (needed <= (int)size && needed >= 0) {
			// It fit fine so we're done.
			return tstring(buf, (size_t)needed);
		}

		// vsnprintf reported that it wanted to write more characters
		// than we allotted.  So try again using a dynamic buffer.  This
		// doesn't happen very often if we chose our initial size well.
		size = (needed > 0) ? (needed + 1) : (size * 2);
		dynamicbuf.resize(size);
		buf = &dynamicbuf[0];
	}
#endif
}






std::string format_time(time_t t = time(nullptr), const char* format = "%a %b %d %H:%M:%S %Y");
std::wstring format_time(time_t t = time(nullptr), const wchar_t* format = L"%a %b %d %H:%M:%S %Y");





template <class T>
std::basic_string<T> socket_address(SOCKET socket) {
	sockaddr_in name;
	socklen_t len = sizeof(name);
	getpeername(socket, (sockaddr*)&name, &len);

	char addr_str[65];
	inet_ntop(name.sin_family, &name.sin_addr, addr_str, sizeof(addr_str));

	return as_basic_string<T, char>(std::string(addr_str));
}





