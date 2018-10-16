#pragma once
#include "../trigger/fsm.h"
#include <stdlib.h>

class GoHome : public trigger::fsm::state
{
public:
	GoHome() : trigger::fsm::state( "GoHome" )
	{
	}

	void begin_state()
	{
	}

	void update(float delta)
	{
		//std::cout << "ImAlive" << std::endl;
	}

	void end_state()
	{
	}
};

class Miner : public trigger::component
{
	trigger::fsm::map *map;
	float time;
	float money;
	float thirst;

public:
	float totalMoney;
	std::string name;

	Miner()
	{
		map = new trigger::fsm::map( new GoHome() );
		map->add_state( new trigger::fsm::state( "EnterMineAndDig" ) );
		map->add_state( new trigger::fsm::state( "VisitBank" ) );
		map->add_state( new trigger::fsm::state( "QuenchThist" ) );

		map->link_state( "GoHome", "EnterMineAndDig" );
		map->link_state( "EnterMineAndDig", "VisitBank" );
		map->link_state( "EnterMineAndDig", "QuenchThist" );
		map->link_state( "VisitBank", "GoHome" );
		map->link_state( "QuenchThist", "EnterMineAndDig" );

		money = 3;
		time = 0;
		thirst = 0;

	}

	std::string get_now_state()
	{
		return map->get_now_state().get_name();
	}

	void update( float delta ) noexcept
	{
		std::string state_name = map->get_now_state().get_name();
		if( state_name == "GoHome" )
		{
			if( money >= 0 )
			{
				money -= delta;
			}
			else
			{
				map->change_link( "GoHome", "EnterMineAndDig", 0 );
			}
		}

		if( state_name == "EnterMineAndDig" )
		{
			money += delta;

			if( money >= 5 )
			{
				map->change_link( "EnterMineAndDig", "VisitBank", 0 );
			}

			if( thirst <= 3 )
			{
				thirst += delta;
			}
			else
			{
				map->change_link( "EnterMineAndDig", "QuenchThist", 0 );
			}
		}

		if( state_name == "QuenchThist" )
		{
			if( thirst >= 0 )
			{
				thirst -= delta;
			}
			else
			{
				map->change_link( "QuenchThist", "EnterMineAndDig", 0 );
			}
		}

		if( state_name == "VisitBank" )
		{

			if( money >= 0 )
			{
				totalMoney += delta;
				money -= delta;
			}
			else
			{
				money = 0;
				map->change_link( "VisitBank", "GoHome", 0 );
			}
		}

		time += delta;
		map->update( delta );
		/*
		std::cout << "Miner time : " << time << std::endl;
		std::cout << "Miner money : " << money << std::endl;
		std::cout << "Miner TotalMoney : " << totalMoney << std::endl;
		std::cout << "Miner thirst : " << thirst << std::endl;
		std::cout << "Miner State : " << map->get_now_state()->get_name() << std::endl;
		system( "cls" );
		*/
	}
};