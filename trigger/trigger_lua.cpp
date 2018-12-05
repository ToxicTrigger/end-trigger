#include "trigger_lua.h"

bool trigger::tlua::is_inited = false;
lua_State *trigger::tlua::L = nullptr;
trigger::ui::console *trigger::tlua::cmd = nullptr;
trigger::component_world *trigger::tlua::world = nullptr;

