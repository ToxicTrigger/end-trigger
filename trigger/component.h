#pragma once
#include <string>
#include "trigger_tools.h"
#include "../json/single_include/nlohmann/json.hpp"

//TODO:: Add using macro in import * export component_world's code. 

// Add name in Component List
// When Component Destroy or Save timing, U Can Use this Macro!
// if registed this component in WorldsUpdateList, this _component are Saving data in file.
// if u dont want save components data? dont call macro in that variable.
// so that list can be import & export names value like {"type", "value"}
// name = value name
// value = name's value
#define SAVE_VAR(var_name) _vars[T_CLASS][#var_name] = {typeid(var_name).name() , var_name };
#define SAVE_VAR_INIT(var_name, var) var_name = var; SAVE_VAR(var_name);


namespace trigger
{
	class component
	{
		using json = nlohmann::json;
	protected:
		json _vars;

	public:
		component()
		{
			SAVE_VAR(time_scale);
			SAVE_VAR(active);
			class_name = (T_CLASS);
		}

		json get_variables()
		{
			return _vars;
		}

		std::string class_name;
		float time_scale = 1.0f;
		bool active = true;
		virtual void update(float delta) noexcept
		{};
	};
}
