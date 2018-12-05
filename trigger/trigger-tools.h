#pragma once
#include <string>
#include <memory>
#include "../json/single_include/nlohmann/json.hpp"

using json = nlohmann::json;

#define T_METHOD methodName(__FUNCTION__).c_str()
#define T_CLASS className(__FUNCTION__).c_str()

inline std::string methodName(const std::string& prettyFunction)
{
	size_t colons = prettyFunction.find("::");
	size_t end = prettyFunction.rfind(" ");

	return prettyFunction.substr(colons + 2, prettyFunction.size()) + "()";
}

inline std::string className(const std::string& prettyFunction)
{
	size_t colons = prettyFunction.rfind("::");
	if (colons == std::string::npos)
		return "::";
	size_t begin = prettyFunction.substr(0, colons).rfind(" ") + 1;
	size_t end = colons - begin;

	return prettyFunction.substr(begin, end);
}
