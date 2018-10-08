#pragma once
#include "../trigger/fsm.h"
#include <stdlib.h>

class Miner : public component
{
	fsm::map *map;
	float time;
	float money;
	float thirst;
	
public:
	float totalMoney;

	Miner()
	{
		map = new fsm::map(new fsm::state("GoHome"));
		map->add_state(new fsm::state("EnterMineAndDig"));
		map->add_state(new fsm::state("VisitBank"));
		map->add_state(new fsm::state("QuenchThist"));
		
		map->link_state("GoHome", "EnterMineAndDig");
		map->link_state("EnterMineAndDig", "VisitBank");
		map->link_state("EnterMineAndDig", "QuenchThist");
		map->link_state("VisitBank", "GoHome");
		map->link_state("QuenchThist", "EnterMineAndDig");


		money = 3;
		time = 0;
		thirst = 0;
	}

	std::string get_now_state()
	{
		return map->get_now_state()->name;
	}

	void update(float delta) noexcept
	{
		if (map->get_now_state()->name == "GoHome")
		{
			if (money >= 0)
			{
				money -= delta;
			}
			else
			{
				map->change_link("GoHome", "EnterMineAndDig", 0);
			}
		}

		if (map->get_now_state()->name == "EnterMineAndDig")
		{
			money += delta;

			if (money >= 5)
			{
				map->change_link("EnterMineAndDig", "VisitBank", 0);
			}

			if (thirst <= 3)
			{
				thirst += delta;
			}
			else 
			{
				map->change_link("EnterMineAndDig", "QuenchThist", 0);
			}
		}

		if (map->get_now_state()->name == "QuenchThist")
		{
			if (thirst >= 0)
			{
				thirst -= delta;
			}
			else
			{
				map->change_link("QuenchThist", "EnterMineAndDig", 0);
			}
		}

		if (map->get_now_state()->name == "VisitBank")
		{
			
			if (money >= 0)
			{
				totalMoney += delta;
				money -= delta;
			}
			else
			{
				money = 0;
				map->change_link("VisitBank", "GoHome", 0);
			}
		}

		time += delta;
		map->simulate();
		std::cout << "Miner time : " << time << std::endl;
		std::cout << "Miner money : " << money << std::endl;
		std::cout << "Miner TotalMoney : " << totalMoney << std::endl;
		std::cout << "Miner thirst : " << thirst << std::endl;
		std::cout << "Miner State : " << map->get_now_state()->name << std::endl;
		system("cls");
	}
};