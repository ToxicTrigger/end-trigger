#ifndef TRIGGER_LUA
#define TRIGGER_LUA

#pragma comment (lib, "lua53.lib")
#include <string>
#include <stdio.h>
#include "trigger_console.h"

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

namespace trigger
{
	class tlua
	{
	private:
		enum error
		{
			None,
			NotInited,
			CantOpen,
			CantSave,
			ScriptError,
		};

		static bool is_inited;
		static lua_State *L;
		static trigger::ui::console *cmd;
		static trigger::component_world *world;

	public:
		// init()			// constructor
		// update(delta)	// tick
		// destroy()		// removed
		static void _load_init_func()
		{
			if (lua_getglobal(tlua::L, "init"))
			{
				lua_pcall(tlua::L, 0, 0, 0);
			}
			else
			{
				tlua::cmd->AddLog("[lua-err] Can't find init() in lua file");
			}	
		}

		static void _load_update_func(float delta)
		{
			if(lua_getglobal(tlua::L, "update"))
			{
				lua_pushnumber(tlua::L, lua_Number(delta));
				lua_pcall(tlua::L, 1, 0, 0);
			}
			else
			{
				tlua::cmd->AddLog("[lua-err] Can't find update() in lua file");
			}
		}

		static void _load_destroy_func()
		{
			if (lua_getglobal(tlua::L, "destroy"))
			{
				lua_pcall(tlua::L, 0, 0, 0);
			}
			else
			{
				tlua::cmd->AddLog("[lua-err] Can't find destroy() in lua file");
			}
		}

		static lua_State *get_lua()
		{
			return tlua::L;
		}

		static tlua::error init(trigger::ui::console *cmd, trigger::component_world *world)
		{
			tlua::L = luaL_newstate();
			tlua::world = world;
			tlua::cmd = cmd;

			luaL_openlibs(L);

			{
				//if u Updated this .h, u must add new func in here!
				lua_register(L, "t_new_actor", t_new_actor);
				lua_register(L, "t_print", t_print);
			}

			is_inited = true;
			return  error::None;
		}

		static tlua::error run(const std::string file)
		{
			if (tlua::is_inited)
			{
				if (!luaL_dofile(tlua::L, file.c_str()))
				{
					tlua::_load_init_func();
					return error::None;
				}
				else 
				{
					tlua::cmd->AddLog("[error] Can't load %s", file.c_str());
					lua_close(tlua::L);
					return error::CantOpen;
				}
			}
			else 
			{
				tlua::cmd->AddLog("[error] Lua is un-inited.. call trigger::tlua::init(...)");
				return error::NotInited;
			}
		}

		//Print msg in console.
		static int t_print(lua_State *L)
		{
			auto msg = lua_tostring(L, 1);
			tlua::cmd->AddLog("[lua-log] %s", msg);
			return 0;
		}

		//Create new actor in world.
		static int t_new_actor(lua_State *L)
		{
			// Get Param String
			auto name = lua_tostring(L, 1);
			tlua::cmd->AddLog("[lua-log] Create New Actor %s", name);

			auto t = new trigger::actor();
			t->name = name;
			tlua::world->add(t);

			lua_pushstring(L, name);
			return 0;
		}
	};
}
#endif