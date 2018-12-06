#include "trigger_lua.h"

bool trigger::tlua::is_inited = false;
lua_State *trigger::tlua::L = nullptr;
trigger::ui::console *trigger::tlua::cmd = nullptr;
trigger::component_world *trigger::tlua::world = nullptr;
TextEditor trigger::tlua::edit;
TextEditor::LanguageDefinition trigger::tlua::lang = TextEditor::LanguageDefinition::Lua();
bool trigger::tlua::lua_editor_init = false;
std::string trigger::tlua::path = "";
bool trigger::tlua::close = false;
trigger::actor *trigger::tlua::target;
