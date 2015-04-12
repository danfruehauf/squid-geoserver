#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string.hpp>

#define MAX_LINE_LENGTH 512

typedef std::map<std::string, std::string> TParameterMap;
typedef std::vector<std::string> TTokens;

void FlipBBox(std::string& a_Bbox)
{
	TTokens Tokens;
	boost::split(Tokens, a_Bbox, boost::is_any_of(","));
	if (4 == Tokens.size()) {
		a_Bbox = Tokens[1] + "," + Tokens[0] + "," + Tokens[3] + "," + Tokens[2];
	}
}

void AddParameter(const std::string& a_Parameter, TParameterMap& a_ParameterMap, const std::string& a_Value, std::string& a_QueryString)
{
	TParameterMap::iterator iter = a_ParameterMap.find(a_Parameter);
	if (a_QueryString.size() != 0) {
		a_QueryString += "&";
	}

	if ("" != a_Value) {
		a_QueryString += a_Parameter + "=" + a_Value;
	} else if (a_ParameterMap.end() != iter) {
		a_QueryString += a_Parameter + "=" + iter->second;
	}
	a_ParameterMap.erase(iter);
}

std::string GenerateQueryString(TParameterMap& a_ParameterMap)
{
	TParameterMap::iterator iter;
	std::string Version, Bbox, QueryString;

	AddParameter("SERVICE", a_ParameterMap, "WMS", QueryString);
	AddParameter("REQUEST", a_ParameterMap, "GetMap", QueryString);

	iter = a_ParameterMap.find("BBOX");
	if (a_ParameterMap.end() != iter) {
		Bbox = iter->second;
		a_ParameterMap.erase(iter);
	}

	iter = a_ParameterMap.find("VERSION");
	if (a_ParameterMap.end() != iter) {
		Version = iter->second;
		if ("1.3.0" == Version) {
			FlipBBox(Bbox);
		}
		QueryString += "&VERSION=1.1.1&BBOX=" + Bbox;
		a_ParameterMap.erase(iter);
	}

	AddParameter("FORMAT", a_ParameterMap, "", QueryString);
	AddParameter("WIDTH", a_ParameterMap, "", QueryString);
	AddParameter("HEIGHT", a_ParameterMap, "", QueryString);
	AddParameter("LAYERS", a_ParameterMap, "", QueryString);

	// Add all the rest of the "unmanaged" parameters
	for (TParameterMap::const_iterator iter2 = a_ParameterMap.begin(); iter2 != a_ParameterMap.end(); ++iter2) {
		QueryString += "&" + iter2->first + "=" + iter2->second;
	}

	return QueryString;
}

bool IsWms(const std::string& a_QueryString)
{
	return a_QueryString.find("/wms?") != -1;
}

void ParseParameters(const std::string& a_UrlString, TParameterMap& a_ParameterMap)
{
	TTokens Tokens;
	boost::split(Tokens, a_UrlString, boost::is_any_of("&"));
	for (TTokens::iterator iter = Tokens.begin(); iter != Tokens.end(); ++iter) {
		TTokens KeyValue;
		boost::split(KeyValue, *iter, boost::is_any_of("="));
		boost::algorithm::to_upper(KeyValue[0]);
		a_ParameterMap[KeyValue[0]] = KeyValue[1];
	}
}

int main(int argc, char** argv)
{
	char Line[MAX_LINE_LENGTH];
	while (std::cin.getline (Line, MAX_LINE_LENGTH)) {
		std::string UrlString(Line);
		if (IsWms(UrlString)) {
			TTokens UrlParts;
			boost::split(UrlParts, UrlString, boost::is_any_of("?"));

			TParameterMap ParameterMap;
			ParseParameters(UrlParts[1], ParameterMap);

			std::cout << "OK store-id=" << UrlParts[0] << "?" << GenerateQueryString(ParameterMap) << std::endl;
		} else {
			std::cout << "ERR" << std::endl;
		}
	}

	return 0;
}
