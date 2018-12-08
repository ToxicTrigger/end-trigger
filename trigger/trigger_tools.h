#pragma once
#include <string>
#include <memory>
#include "imgui.h"

#define T_METHOD methodName(__FUNCTION__).c_str()
#define T_CLASS className(__FUNCTION__).c_str()

#define ASSERT(expr, msg, add) \
static bool click = false;\
if( !expr && !click ) \
{\
ImGui::Begin("Error");\
ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.4f, 1.0f), msg, add);\
if(ImGui::ColorButton("Close", ImVec4(0.5f, 0.4f, 0.2f, 1.0f)))\
click = true;\
ImGui::End();\
}

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
