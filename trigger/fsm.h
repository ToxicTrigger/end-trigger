#pragma once
#include <thread>
#include <string>
#include <vector>
#include "component.h"
#include <iostream>

namespace fsm 
{
	class state
	{
	public:
		std::string name;

		inline state() noexcept
		{
			this->name = "Unknown";
		}

		inline state(std::string name) noexcept
		{
			this->name = name;
		}
	};

	class link
	{
	public:
		state *cur;
		state *next;
		int ops;

		link()
		{
			cur = nullptr;
			next = nullptr;
			ops = 1;
		}

		link(state *current, state *next) : link()
		{
			this->cur = current;
			this->next = next;
		}

	};

	class map : public component
	{
	public:
		std::vector<state*> states;
		std::vector<link*> links;
		state *now_state;
		
		map()
		{
			states = std::vector<state*>();
			links = std::vector<link*>();
			state *idle = new state("idle");
			add_state(idle);
			now_state = idle;
		}

		map(state *def_state) : map()
		{
			// inited state idle
			add_state(def_state);
			link *def = new link(now_state, def_state);
			def->ops = 0;
			links.push_back(def);
		}

		inline state* get_state(unsigned int index) const noexcept
		{
			if (index >= states.size()) return nullptr;
			return states[index];
		}

		inline state* get_state(std::string name) const noexcept
		{
			for (auto i : states)
			{
				if (i->name == name)
				{
					return i;
				}
			}
			return nullptr;
		}

		inline state* get_state(state *state)
		{
			for (auto i : states)
			{
				if (i == state) return i;
			}
			return nullptr;
		}

		inline bool link_state(std::string state1, std::string state2) noexcept
		{
			state *a = get_state(state1);
			state *b = get_state(state2);
			if (a == nullptr || b == nullptr)
			{
				return false;
			}
			link *tmp = new link(a, b);
			this->links.push_back(tmp);

			return true;
		}

		inline void add_state(state *new_state) noexcept
		{
			if (new_state != nullptr)
			{
				states.push_back(new_state);
			}
		}

		inline void simulate() noexcept
		{
			for (auto i : this->links)
			{
				if (i->cur->name == now_state->name)
				{
					// ops == 0 , move now_state between link
					if (i->ops == 0)
					{
						i->ops += 1;
						now_state = i->next;

						return;
					}
				}
			}
		}

		inline bool change_link(std::string state1, std::string state2, unsigned int op) const noexcept
		{
			state *tmp = get_state(state1);
			state *tmp2 = get_state(state2);
			if (tmp != nullptr && tmp2 != nullptr)
			{
				for (auto i : links)
				{
					if (i->cur->name == tmp->name && i->next->name == tmp2->name)
					{
						i->ops = op;
					}
				}
				return true;
			}
			return false;
		}

		void update(float delta) noexcept
		{
			simulate();
			std::cout << "now_state : " << now_state->name << std::endl;
		}

		~map()
		{
			if (now_state != nullptr)
			{
				delete now_state;
			}

			states.clear();
			links.clear();
		}

	};

}
