//
// Created by root on 9/4/18.
//
#ifndef IPDB_IPDB_H
#define IPDB_IPDB_H

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace ipdb {
#define  IPv4  0x01
#define  IPv6  0x02
#define ErrFileSize "IP Database file size error."
#define ErrMetaData "IP Database metadata error."
//#define ErrReadFull "IP Database ReadFull error."
#define ErrDatabaseError "database error"
#define ErrIPFormat "Query IP Format error."
#define ErrNoSupportLanguage "language not support"
#define ErrNoSupportIPv4 "IPv4 not support"
#define ErrNoSupportIPv6 "IPv6 not support"
#define ErrDataNotExists "data is not exists"
    using namespace std;

    class MetaData {
    public:
        uint64_t Build{};             //`json:"build"`
        uint16_t IPVersion{};         //`json:"ip_version"`
        map<string, int> Languages; //`json:"languages"`
        int NodeCount{};              //`json:"node_count"`
        int TotalSize{};              //`json:"total_size"`
        vector<string> Fields;      //`json:"fields"`
        void Parse(const string &json);
    };

    class Reader {
        MetaData meta;
        int v4offset = 0;
        int fileSize = 0;
        int dataSize = 0;
        shared_ptr<u_char> data = nullptr;

        int readNode(int node, int index) const;

        string resolve(int node);

        int search(const u_char *ip, int bitCount) const;

        string find0(const string &addr);

        vector<string> find1(const string &addr, const string &language);

    public:
        ~Reader();

        explicit Reader(const string &file);

        vector<string> Find(const string &addr, const string &language);

        map<string, string> FindMap(const string &addr, const string &language);

        bool IsIPv4Support() const;

        bool IsIPv6Support() const;

        uint64_t BuildTime() const;

        vector<string> Languages();

        vector<string> Fields() const;
    };

    class ASNInfo {
        string asn;
        string reg;
        string cc;
        string net;
        string org;
        string type;
        string domain;
    public:
        explicit ASNInfo(const vector<string> &data, const vector<string> &fields);

        string GetAsn();

        string GetReg();

        string GetCc();

        string GetNet();

        string GetOrg();

        string GetType();

        string GetDomain();

        string str();
    };

    class DistrictInfo {
        string country_name;
        string region_name;
        string city_name;
        string district_name;
        string china_admin_code;
        string covering_radius;
        string latitude;
        string longitude;
    public:
        explicit DistrictInfo(const vector<string> &data, const vector<string> &fields);

        string GetCountryName();

        string GetRegionName();

        string GetCityName();

        string GetDistrictName();

        string GetChinaAdminCode();

        string GetCoveringRadius();

        string GetLatitude();

        string GetLongitude();

        string str();
    };

    class District : public Reader {
    public:
        explicit District(const string &file);

        DistrictInfo FindInfo(const string &addr, const string &language);
    };

    class CityInfo {
        string country_name;
        string region_name;
        string city_name;
        string owner_domain;
        string isp_domain;
        string latitude;
        string longitude;
        string timezone;
        string utc_offset;
        string china_admin_code;
        string idd_code;
        string country_code;
        string continent_code;
        string idc;
        string base_station;
        string country_code3;
        string european_union;
        string currency_code;
        string currency_name;
        string anycast;
        string line;
        shared_ptr<DistrictInfo> district_info;
        string route;
        string asn;
        vector<shared_ptr<ASNInfo>> asn_info;
        string area_code;
        string usage_type;
    public:
        explicit CityInfo(const vector<string> &data, const vector<string> &fields);

        string GetCountryName();

        string GetRegionName();

        string GetCityName();

        string GetOwnerDomain();

        string GetIspDomain();

        string GetLatitude();

        string GetLongitude();

        string GetTimezone();

        string GetUtcOffset();

        string GetChinaAdminCode();

        string GetIddCode();

        string GetCountryCode();

        string GetContinentCode();

        string GetIDC();

        string GetBaseStation();

        string GetCountryCode3();

        string GetEuropeanUnion();

        string GetCurrencyCode();

        string GetCurrencyName();

        string GetAnycast();

        string GetLine();

        shared_ptr<DistrictInfo> GetDistrictInfo();

        string GetRoute();

        string GetASN();

        vector<shared_ptr<ASNInfo>> GetASNInfo();

        string GetAreaCode();

        string GetUsageType();

        string str();
    };

    class City : public Reader {
    public:
        explicit City(const string &file);

        CityInfo FindInfo(const string &addr, const string &language);
    };

    class BaseStationInfo {
        string country_name;
        string region_name;
        string city_name;
        string owner_domain;
        string isp_domain;
        string base_station;
    public:
        explicit BaseStationInfo(const vector<string> &data, const vector<string> &fields);

        string GetCountryName();

        string GetRegionName();

        string GetCityName();

        string GetOwnerDomain();

        string GetIspDomain();

        string GetBaseStation();

        string str();
    };

    class BaseStation : public Reader {
    public:
        explicit BaseStation(const string &file);

        BaseStationInfo FindInfo(const string &addr, const string &language);
    };

    class IDCInfo {
        string country_name;
        string region_name;
        string city_name;
        string owner_domain;
        string isp_domain;
        string idc;
    public:
        explicit IDCInfo(const vector<string> &data, const vector<string> &fields);

        string GetCountryName();

        string GetRegionName();

        string GetCityName();

        string GetOwnerDomain();

        string GetIspDomain();

        string GetIDC();

        string str();
    };

    class IDC : public Reader {
    public:
        explicit IDC(const string &file);

        IDCInfo FindInfo(const string &addr, const string &language);
    };
}
#endif //IPDB_IPDB_H
