#ifndef TRIGGER_LUA
#define TRIGGER_LUA

#pragma comment (lib, "lua53.lib")
#include <string>
#include <stdio.h>
#include <fstream>
#include "trigger_console.h"
#include "TextEditor.h"
#include "d3dApp.h"

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
		static trigger::actor *target;
		static TextEditor edit;
		static TextEditor::LanguageDefinition lang;
		static bool lua_editor_init;
		static std::string path;
		static bool close;

	public:
		static bool open_lua_editor(std::string path, bool* window)
		{
			if (!tlua::path.compare(path))
			{
				tlua::lua_editor_init = false;
				tlua::path = path;
			}

			if (!tlua::lua_editor_init)
			{
				edit.SetLanguageDefinition(lang);
				std::ifstream i(path);
				if (i.good())
				{
					std::string str((std::istreambuf_iterator<char>(i)), std::istreambuf_iterator<char>());
					edit.SetText(str);
					tlua::lua_editor_init = true;

				}
			}

			auto cpos = edit.GetCursorPosition();
			ImGui::Begin("Trigger Editor", nullptr, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_MenuBar);
			ImGui::SetWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (!edit.IsReadOnly())
					{
						if (ImGui::MenuItem("Save", "Ctrl-S", nullptr, true))
						{
							auto textToSave = edit.GetText();
							/// save text....
							ofstream o(path);
							o << textToSave;
							o.close();
						}
					}
					if (ImGui::MenuItem("Quit", "Alt-F4"))
					{
						ImGui::EndMenu();
						ImGui::EndMenuBar();
						ImGui::End();
						*window = false;
						return true;
					}

					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Edit"))
				{
					bool ro = edit.IsReadOnly();
					if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
						edit.SetReadOnly(ro);
					ImGui::Separator();

					if (ImGui::MenuItem("Undo", "ALT-Backspace", nullptr, !ro && edit.CanUndo()))
						edit.Undo();
					if (ImGui::MenuItem("Redo", "Ctrl-Y", nullptr, !ro && edit.CanRedo()))
						edit.Redo();

					ImGui::Separator();

					if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, edit.HasSelection()))
						edit.Copy();
					if (ImGui::MenuItem("Cut", "Ctrl-X", nullptr, !ro && edit.HasSelection()))
						edit.Cut();
					if (ImGui::MenuItem("Delete", "Del", nullptr, !ro && edit.HasSelection()))
						edit.Delete();
					if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
						edit.Paste();

					ImGui::Separator();

					if (ImGui::MenuItem("Select all", nullptr, nullptr))
						edit.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(edit.GetTotalLines(), 0));

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("View"))
				{
					if (ImGui::MenuItem("Dark palette"))
						edit.SetPalette(TextEditor::GetDarkPalette());
					if (ImGui::MenuItem("Light palette"))
						edit.SetPalette(TextEditor::GetLightPalette());
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			ImGui::Text("%6d/%-6d %6d lines  | %s | %s | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, edit.GetTotalLines(),
				edit.IsOverwrite() ? "Ovr" : "Ins",
				edit.CanUndo() ? "*" : " ",
				edit.GetLanguageDefinition().mName.c_str(), tlua::path.c_str());

			edit.Render("TextEditor");
			ImGui::End();

		}

		static bool save_default_lua_file(std::string path)
		{
			//Read Default lua file and copy
			std::ifstream i("lua/lua_default");
			if (i.is_open())
			{
				std::string data;
				std::string chunk;
				while (std::getline(i, data))
				{
					chunk += data;
					chunk.push_back('\n');
				}

				ofstream o(path);
				if (o.is_open())
				{
					o << chunk;
					o.close();
					return true;
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
		// init()			// constructor
		// update(delta)	// tick
		// destroy()		// removed
		static void _load_init_func()
		{
			if (close == false)
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
		}

		static void _load_update_func(float delta)
		{
			if (close == false)
			{
				if (lua_getglobal(tlua::L, "update"))
				{
					lua_pushnumber(tlua::L, lua_Number(delta));
					lua_pcall(tlua::L, 1, 0, 0);
				}
				else
				{
					tlua::cmd->AddLog("[lua-err] Can't find update() in lua file");
				}
			}
		}

		static void _load_destroy_func()
		{
			if (lua_getglobal(tlua::L, "destroy"))
			{
				lua_pcall(tlua::L, 0, 0, 0);
				tlua::close = true;
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

		static tlua::error init(trigger::ui::console *cmd, trigger::component_world *world, trigger::actor *target)
		{
			tlua::L = luaL_newstate();
			tlua::world = world;
			tlua::cmd = cmd;
			tlua::target = target;

			luaL_openlibs(L);

			{
				//if u Updated this .h, u must add new func in here!
				lua_register(L, "t_new_actor", t_new_actor);
				lua_register(L, "t_print", t_print);
				lua_register(L, "t_rotation", t_rotation);
				lua_register(L, "t_set_rotation", t_set_rotation);
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
					close = false;
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
		//set rotate actor
		static int t_set_rotation(lua_State *L)
		{
			std::string name = lua_tostring(L, 1);
			float x = (float)(lua_tonumber(L, 2));
			float y = (float)lua_tonumber(L, 3);
			float z = (float)lua_tonumber(L, 4);
			tlua::target->s_transform.rotation.x = x;
			tlua::target->s_transform.rotation.y = y;
			tlua::target->s_transform.rotation.z = z;
			return 0;
		}

		//rotate actor
		static int t_rotation(lua_State *L)
		{
			std::string name = lua_tostring(L, 1);
			float x = (float)(lua_tonumber(L, 2));
			float y = (float)lua_tonumber(L, 3);
			float z = (float)lua_tonumber(L, 4);
			tlua::target->s_transform.rotation.x += x;
			tlua::target->s_transform.rotation.y += y;
			tlua::target->s_transform.rotation.z += z;
			return 0;
		}
	};
}
#endif