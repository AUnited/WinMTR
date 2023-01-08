//*****************************************************************************
// FILE:            WinMTRGlobal.cpp
//
//
//*****************************************************************************

#include "WinMTRGlobal.h"

//*****************************************************************************
// gettimeofday
//
// win32 port of unix gettimeofday
//*****************************************************************************
/*
int gettimeofday(struct timeval* tv, struct timezone* / *tz* /)
{
   if(!tv)
      return -1;
   struct _timeb timebuffer;
   
   _ftime(&timebuffer);

   tv->tv_sec = (long)timebuffer.time;
   tv->tv_usec = timebuffer.millitm * 1000 + 500;
   return 0;
}// */
#include <codecvt>
#include <numeric>

#include "ipdb.cpp"

std::vector<ipdb::City> dbs;


// convert UTF-8 string to wstring
std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> myconv;
	return myconv.from_bytes(str);
}

// convert wstring to UTF-8 string
std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<codecvt_utf8_utf16<wchar_t>> myconv;
	return myconv.to_bytes(str);
}

_tstring utf8_to_tstring(const std::string &str)
{
#ifdef UNICODE
	return utf8_to_wstring(str);
#else
	return str;
#endif
}

void ip2loc_init()
{
	CHAR* db_names[2] = { ("17monipdbv4.ipdb"), ("17monipdbv6.ipdb") };
	for (const auto db_name: db_names)
	{
		try {
			auto db = ipdb::City(db_name);
			dbs.push_back(std::move(db));
		} catch (const char *)
		{
			// do nothing
		}
	}
}

bool ip2loc_ischinese()
{
	for (auto &db: dbs)
	{
		for (auto &lang : db.Languages())
		{
			if (lang == "CN")
			{
				return true;
			}
		}
	}
	return false;
}

std::string myconcat(const std::string &lhs, const std::string &rhs)
{
	if (lhs.size() < 2 || rhs.empty())
	{
		return lhs + " " + rhs;
	}
	// detect if last character unicode character
	if (((uint8_t) lhs[lhs.size() - 2] & 0x80) && ((uint8_t) rhs[0] & 0x80))
	{
		return lhs + rhs;
	}
	return lhs + " " + rhs;
}

_tstring ip2loc_lookup(const std::string &addr)
{
	if (dbs.empty())
	{
		return _T("<No DB available>");
	}
	for (auto &db: dbs)
	{
		if (db.IsIPv4Support() && addr.length() == 4 || db.IsIPv6Support() && addr.length() == 16) {
			auto info = db.FindInfo(addr, db.Languages()[0]);
			OutputDebugString(utf8_to_wstring(info.str()).c_str());

			vector<std::string> out;
			out.push_back(info.GetCountryName());
			for (auto &i : {info.GetRegionName(), info.GetCityName(), info.GetIspDomain()})
			{
				if (i.empty() || i == out.back()) continue;
				out.push_back(std::move(i));
			}
			auto output = std::accumulate(out.cbegin(), out.cend(), string(), myconcat);
			auto ret = utf8_to_wstring(output);
			return ret;
		}
	}
	return _T("<DB doesn't support this kind of IP>");
}