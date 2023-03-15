//
// Created by root on 9/4/18.
//
#include "ipdb.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <fstream>
#include <sstream>
// TODO: you can re-target windows server 2003 via changing inet_pton call to getaddrinfo
#include <WS2tcpip.h>
//#include <arpa/inet.h>

using namespace std;
using namespace rapidjson;

void ipdb::MetaData::Parse(const string &json) {
    Document d;
    d.Parse(json.c_str());
    Build = d["build"].GetUint64();
    IPVersion = static_cast<uint16_t>(d["ip_version"].GetUint());
    NodeCount = d["node_count"].GetInt();
    TotalSize = d["total_size"].GetInt();
    auto fields = d["fields"].GetArray();
    for (auto &field : fields) {
        Fields.emplace_back(field.GetString());
    }
    auto languages = d["languages"].GetObject();
    for (auto &language:languages) {
        Languages[language.name.GetString()] = language.value.GetInt();
    }
}

int ipdb::Reader::readNode(int node, int index) const {
    auto off = node * 8 + index * 4;
    return ntohl(static_cast<uint32_t>(*(int *) &data.get()[off]));
}

string ipdb::Reader::resolve(int node) {
    auto resolved = node - meta.NodeCount + meta.NodeCount * 8;
    if (resolved >= fileSize) {
        throw ErrDatabaseError;
    }
    auto size = (data.get()[resolved] << 8) | data.get()[resolved + 1];
    if ((resolved + 2 + size) > dataSize) {
        throw ErrDatabaseError;
    }
    string bytes = (const char *) data.get() + resolved + 2;
    return bytes;
}

int ipdb::Reader::search(const u_char *ip, int bitCount) const {
    int node = 0;
    if (bitCount == 32) {
        node = v4offset;
    } else {
        node = 0;
    }
    for (auto i = 0; i < bitCount; ++i) {
        if (node > meta.NodeCount) {
            break;
        }
        node = readNode(node, ((0xFF & int(ip[i >> 3])) >> unsigned int(7 - (i % 8))) & 1);
    }
    if (node > meta.NodeCount) {
        return node;
    }
    throw ErrDataNotExists;
}

string ipdb::Reader::find0(const string &addr) {
    /*int node = 0;
    struct in_addr addr4{};
    struct in6_addr addr6{};
    if (inet_pton(AF_INET, addr.c_str(), &addr4)) {
        if (!IsIPv4Support()) {
            throw ErrNoSupportIPv4;
        }
        node = search((const u_char *) &addr4.s_addr, 32);
    } else if (inet_pton(AF_INET6, addr.c_str(), &addr6)) {
        if (!IsIPv6Support()) {
            throw ErrNoSupportIPv6;
        }
        node = search((const u_char *) &addr6.s6_addr, 128);
    } else {
        throw ErrIPFormat;
    }*/
    // We changed it to use binary IP address
    if (addr.length() != 4 && addr.length() != 16)
    {
        throw ErrIPFormat;
    }
    int node = search((const u_char *)addr.data(), addr.length() * 8);
    auto body = resolve(node);
    return body;
}

vector<string> split(const string &s, const string &sp) {
    vector<string> output;
    string::size_type prev_pos = 0, pos = 0;
    while ((pos = s.find(sp, pos)) != string::npos) {
        string substring(s.substr(prev_pos, pos - prev_pos));
        output.emplace_back(substring);
        prev_pos = ++pos;
    }
    output.emplace_back(s.substr(prev_pos, pos - prev_pos)); // Last word
    return output;
}

vector<string> ipdb::Reader::find1(const string &addr, const string &language) {
    if (meta.Languages.find(language) == meta.Languages.end()) {
        throw ErrNoSupportLanguage;
    }
    auto off = meta.Languages[language];
    auto body = find0(addr);
    auto tmp = split(body, "\t");
    if (off + meta.Fields.size() > tmp.size()) {
        throw ErrDatabaseError;
    }
    vector<string> result;
    for (auto i = tmp.begin() + off; i != tmp.begin() + off + meta.Fields.size(); ++i) {
        result.emplace_back(*i);
    }
    return result;
}

map<string, string> ipdb::Reader::FindMap(const string &addr, const string &language) {
    auto res = find1(addr, language);
    map<string, string> info;
    auto k = 0;
    for (auto &v : res) {
        info[meta.Fields[k++]] = v;
    }
    return info;
}

vector<string> ipdb::Reader::Find(const string &addr, const string &language) {
    return find1(addr, language);
}

bool ipdb::Reader::IsIPv6Support() const {
    return (meta.IPVersion & IPv6) == IPv6;
}

bool ipdb::Reader::IsIPv4Support() const {
    return (meta.IPVersion & IPv4) == IPv4;
}

ipdb::Reader::Reader(const string &file) {
    ifstream fs(file, ios::binary | ios::in);
    if (fs.tellg() == -1) {
        throw ErrFileSize;
    }
    fs.seekg(0, ios::end);
    auto fsize = fs.tellg();
    uint32_t metaLength = 0ul;
    fs.seekg(0, ios::beg);
    fs.read((char *) &metaLength, 4);
    metaLength = ntohl(metaLength);
    string bb;
    bb.resize(metaLength);
    fs.read(&bb[0], metaLength);
    meta.Parse(bb);
    if (meta.Languages.empty() || meta.Fields.empty()) {
        throw ErrMetaData;
    }
    if (fsize != (4 + metaLength + meta.TotalSize)) {
        throw ErrFileSize;
    }
    fileSize = (int) fsize;
    auto dataLen = (int) fsize - 4 - metaLength;
    data = shared_ptr<u_char>(new u_char[dataLen], std::default_delete<u_char[]>());;
    fs.read((char *) data.get(), dataLen);
    dataSize = dataLen;
    auto node = 0;
    for (auto i = 0; i < 96 && node < meta.NodeCount; ++i) {
        if (i >= 80) {
            node = readNode(node, 1);
        } else {
            node = readNode(node, 0);
        }
    }
    v4offset = node;
}

ipdb::Reader::~Reader() = default;

uint64_t ipdb::Reader::BuildTime() const {
    return meta.Build;
}

vector<string> ipdb::Reader::Languages() {
    vector<string> ls;
    for (const auto &i:meta.Languages) {
        ls.emplace_back(i.first);
    }
    return ls;
}

vector<string> ipdb::Reader::Fields() const {
    return meta.Fields;
}

ipdb::ASNInfo::ASNInfo(const vector<string> &data, const vector<string> &fields) {
    auto i = fields.begin();
    auto j = data.begin();
    for (; i != fields.end() && j != data.end(); ++i, ++j) {
        if (*i == "asn")asn = *j;
        else if (*i == "reg")reg = *j;
        else if (*i == "cc")cc = *j;
        else if (*i == "net")net = *j;
        else if (*i == "org")org = *j;
        else if (*i == "type")type = *j;
        else if (*i == "domain")domain = *j;
    }
}

string ipdb::ASNInfo::GetAsn() { return asn; }

string ipdb::ASNInfo::GetReg() { return reg; }

string ipdb::ASNInfo::GetCc() { return cc; }

string ipdb::ASNInfo::GetNet() { return net; }

string ipdb::ASNInfo::GetOrg() { return org; }

string ipdb::ASNInfo::GetType() { return type; }

string ipdb::ASNInfo::GetDomain() { return domain; }

string ipdb::ASNInfo::str() {
    stringstream sb;
    sb << "asn: " << asn << endl;
    sb << "reg: " << reg << endl;
    sb << "cc: " << cc << endl;
    sb << "net: " << net << endl;
    sb << "org: " << org << endl;
    sb << "type: " << type << endl;
    sb << "domain: " << domain << endl;
    return sb.str();
}

ipdb::CityInfo::CityInfo(const vector<string> &data, const vector<string> &fields) {
    auto i = fields.begin();
    auto j = data.begin();
    for (; i != fields.end() && j != data.end(); ++i, ++j) {
        if (*i == "country_name") { country_name = *j; }
        else if (*i == "region_name") { region_name = *j; }
        else if (*i == "city_name") { city_name = *j; }
        else if (*i == "owner_domain") { owner_domain = *j; }
        else if (*i == "isp_domain") { isp_domain = *j; }
        else if (*i == "latitude") { latitude = *j; }
        else if (*i == "longitude") { longitude = *j; }
        else if (*i == "timezone") { timezone = *j; }
        else if (*i == "utc_offset") { utc_offset = *j; }
        else if (*i == "china_admin_code") { china_admin_code = *j; }
        else if (*i == "idd_code") { idd_code = *j; }
        else if (*i == "country_code") { country_code = *j; }
        else if (*i == "continent_code") { continent_code = *j; }
        else if (*i == "idc") { idc = *j; }
        else if (*i == "base_station") { base_station = *j; }
        else if (*i == "country_code3") { country_code3 = *j; }
        else if (*i == "european_union") { european_union = *j; }
        else if (*i == "currency_code") { currency_code = *j; }
        else if (*i == "currency_name") { currency_name = *j; }
        else if (*i == "anycast") { anycast = *j; }
        else if (*i == "line") { line = *j; }
        else if (*i == "district_info") {
            vector<string> names;
            vector<string> values;
            if (!j->empty()) {
                Document doc;
                doc.Parse(j->c_str());
                if (doc.IsObject()) {
                    for (const auto &o: doc.GetObject()) {
                        values.emplace_back(o.name.GetString());
                        names.emplace_back(o.value.GetString());
                    }
                }
            }
            district_info = make_shared<ipdb::DistrictInfo>(names, values);
        } else if (*i == "route") { route = *j; }
        else if (*i == "asn") { asn = *j; }
        else if (*i == "asn_info") {
            vector<shared_ptr<ipdb::ASNInfo>> ans_vector;
            if (!j->empty()) {
                Document doc;
                doc.Parse(j->c_str());
                if (doc.IsObject()) {
                    for (const auto &array:doc.GetArray()) {
                        vector<string> names;
                        vector<string> values;
                        for (const auto &o:array.GetObject()) {
                            values.emplace_back(o.name.GetString());
                            names.emplace_back(o.value.GetString());
                        }
                        ans_vector.emplace_back(make_shared<ipdb::ASNInfo>(names, values));
                    }
                }
            }
            asn_info = move(ans_vector);
        } else if (*i == "area_code") { area_code = *j; }
        else if (*i == "usage_type") { usage_type = *j; }
    }
}

string ipdb::CityInfo::GetCountryName() { return country_name; }

string ipdb::CityInfo::GetRegionName() { return region_name; }

string ipdb::CityInfo::GetCityName() { return city_name; }

string ipdb::CityInfo::GetOwnerDomain() { return owner_domain; }

string ipdb::CityInfo::GetIspDomain() { return isp_domain; }

string ipdb::CityInfo::GetLatitude() { return latitude; }

string ipdb::CityInfo::GetLongitude() { return longitude; }

string ipdb::CityInfo::GetTimezone() { return timezone; }

string ipdb::CityInfo::GetUtcOffset() { return utc_offset; }

string ipdb::CityInfo::GetChinaAdminCode() { return china_admin_code; }

string ipdb::CityInfo::GetIddCode() { return idd_code; }

string ipdb::CityInfo::GetCountryCode() { return country_code; }

string ipdb::CityInfo::GetContinentCode() { return continent_code; }

string ipdb::CityInfo::GetIDC() { return idc; }

string ipdb::CityInfo::GetBaseStation() { return base_station; }

string ipdb::CityInfo::GetCountryCode3() { return country_code3; }

string ipdb::CityInfo::GetEuropeanUnion() { return european_union; }

string ipdb::CityInfo::GetCurrencyCode() { return currency_code; }

string ipdb::CityInfo::GetCurrencyName() { return currency_name; }

string ipdb::CityInfo::GetAnycast() { return anycast; }

string ipdb::CityInfo::GetLine() { return line; }

shared_ptr<ipdb::DistrictInfo> ipdb::CityInfo::GetDistrictInfo() { return district_info; }

string ipdb::CityInfo::GetRoute() { return route; }

string ipdb::CityInfo::GetASN() { return asn; }

vector<shared_ptr<ipdb::ASNInfo>> ipdb::CityInfo::GetASNInfo() { return asn_info; }

string ipdb::CityInfo::GetAreaCode() { return area_code; }

string ipdb::CityInfo::GetUsageType() { return usage_type; }

string ipdb::CityInfo::str() {
    stringstream sb;
    sb << "country_name: " << country_name << endl;
    sb << "region_name: " << region_name << endl;
    sb << "city_name: " << city_name << endl;
    sb << "owner_domain: " << owner_domain << endl;
    sb << "isp_domain: " << isp_domain << endl;
    sb << "latitude: " << latitude << endl;
    sb << "longitude: " << longitude << endl;
    sb << "timezone: " << timezone << endl;
    sb << "utc_offset: " << utc_offset << endl;
    sb << "china_admin_code: " << china_admin_code << endl;
    sb << "idd_code: " << idd_code << endl;
    sb << "country_code: " << country_code << endl;
    sb << "continent_code: " << continent_code << endl;
    sb << "idc: " << idc << endl;
    sb << "base_station: " << base_station << endl;
    sb << "country_code3: " << country_code3 << endl;
    sb << "european_union: " << european_union << endl;
    sb << "currency_code: " << currency_code << endl;
    sb << "currency_name: " << currency_name << endl;
    sb << "anycast: " << anycast << endl;
    sb << "line: " << line << endl;
    if (district_info) sb << "district_info: " << district_info->str() << endl;
    sb << "route: " << route << endl;
    sb << "asn: " << asn << endl;
    for (const auto &i:asn_info)
        sb << "asn_info: " << i->str() << endl;
    sb << "area_code: " << area_code << endl;
    sb << "usage_type: " << usage_type << endl;
    return sb.str();
}

ipdb::City::City(const string &file) : Reader(file) {}

ipdb::CityInfo ipdb::City::FindInfo(const string &addr, const string &language) {
    return CityInfo(Find(addr, language), this->Fields());
}

ipdb::BaseStationInfo::BaseStationInfo(const vector<string> &data, const vector<string> &fields) {
    auto i = fields.begin();
    auto j = data.begin();
    for (; i != fields.end() && j != data.end(); ++i, ++j) {
        if (*i == "country_name") { country_name = *j; }
        else if (*i == "region_name") { region_name = *j; }
        else if (*i == "city_name") { city_name = *j; }
        else if (*i == "owner_domain") { owner_domain = *j; }
        else if (*i == "isp_domain") { isp_domain = *j; }
        else if (*i == "base_station") { base_station = *j; }
    }
}

string ipdb::BaseStationInfo::GetCountryName() { return country_name; }

string ipdb::BaseStationInfo::GetRegionName() { return region_name; }

string ipdb::BaseStationInfo::GetCityName() { return city_name; }

string ipdb::BaseStationInfo::GetOwnerDomain() { return owner_domain; }

string ipdb::BaseStationInfo::GetIspDomain() { return isp_domain; }

string ipdb::BaseStationInfo::GetBaseStation() { return base_station; }

string ipdb::BaseStationInfo::str() {
    stringstream sb;
    sb << "country_name: " << country_name << endl;
    sb << "region_name: " << region_name << endl;
    sb << "city_name: " << city_name << endl;
    sb << "owner_domain: " << owner_domain << endl;
    sb << "isp_domain: " << isp_domain << endl;
    sb << "base_station: " << base_station << endl;
    return sb.str();
}

ipdb::BaseStation::BaseStation(const string &file) : Reader(file) {}

ipdb::BaseStationInfo ipdb::BaseStation::FindInfo(const string &addr, const string &language) {
    return BaseStationInfo(Find(addr, language), this->Fields());
}

ipdb::DistrictInfo::DistrictInfo(const vector<string> &data, const vector<string> &fields) {
    auto i = fields.begin();
    auto j = data.begin();
    for (; i != fields.end() && j != data.end(); ++i, ++j) {
        if (*i == "country_name") { country_name = *j; }
        else if (*i == "region_name") { region_name = *j; }
        else if (*i == "city_name") { city_name = *j; }
        else if (*i == "district_name") { district_name = *j; }
        else if (*i == "china_admin_code") { china_admin_code = *j; }
        else if (*i == "covering_radius") { covering_radius = *j; }
        else if (*i == "latitude") { latitude = *j; }
        else if (*i == "longitude") { longitude = *j; }
    }
}

string ipdb::DistrictInfo::GetCountryName() { return country_name; }

string ipdb::DistrictInfo::GetRegionName() { return region_name; }

string ipdb::DistrictInfo::GetCityName() { return city_name; }

string ipdb::DistrictInfo::GetDistrictName() { return district_name; }

string ipdb::DistrictInfo::GetChinaAdminCode() { return china_admin_code; }

string ipdb::DistrictInfo::GetCoveringRadius() { return covering_radius; }

string ipdb::DistrictInfo::GetLatitude() { return latitude; }

string ipdb::DistrictInfo::GetLongitude() { return longitude; }

string ipdb::DistrictInfo::str() {
    stringstream sb;
    sb << "country_name: " << country_name << endl;
    sb << "region_name: " << region_name << endl;
    sb << "city_name: " << city_name << endl;
    sb << "district_name: " << district_name << endl;
    sb << "china_admin_code: " << china_admin_code << endl;
    sb << "covering_radius: " << covering_radius << endl;
    sb << "latitude: " << latitude << endl;
    sb << "longitude: " << longitude << endl;
    return sb.str();
}

ipdb::District::District(const string &file) : Reader(file) {}

ipdb::DistrictInfo ipdb::District::FindInfo(const string &addr, const string &language) {
    return DistrictInfo(Find(addr, language), this->Fields());
}

ipdb::IDCInfo::IDCInfo(const vector<string> &data, const vector<string> &fields) {
    auto i = fields.begin();
    auto j = data.begin();
    for (; i != fields.end() && j != data.end(); ++i, ++j) {
        if (*i == "country_name") { country_name = *j; }
        else if (*i == "region_name") { region_name = *j; }
        else if (*i == "city_name") { city_name = *j; }
        else if (*i == "owner_domain") { owner_domain = *j; }
        else if (*i == "isp_domain") { isp_domain = *j; }
        else if (*i == "idc") { idc = *j; }
    }
}

string ipdb::IDCInfo::GetCountryName() { return country_name; }

string ipdb::IDCInfo::GetRegionName() { return region_name; }

string ipdb::IDCInfo::GetCityName() { return city_name; }

string ipdb::IDCInfo::GetOwnerDomain() { return owner_domain; }

string ipdb::IDCInfo::GetIspDomain() { return isp_domain; }

string ipdb::IDCInfo::GetIDC() { return idc; }

string ipdb::IDCInfo::str() {
    stringstream sb;
    sb << "country_name: " << country_name << endl;
    sb << "region_name: " << region_name << endl;
    sb << "city_name: " << city_name << endl;
    sb << "owner_domain: " << owner_domain << endl;
    sb << "isp_domain: " << isp_domain << endl;
    sb << "idc: " << idc << endl;
    return sb.str();
}

ipdb::IDC::IDC(const string &file) : Reader(file) {}

ipdb::IDCInfo ipdb::IDC::FindInfo(const string &addr, const string &language) {
    return IDCInfo(Find(addr, language), this->Fields());
}
