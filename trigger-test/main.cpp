#include "Miner.h"
#include "../trigger/component_world.h"

using namespace std;

auto main() -> int
{
	auto miner = make_unique<Miner>();
	miner->name = "1";
	auto miner1 = make_unique<Miner>();
	miner1->name = "2";
	trigger::component_world *world = new trigger::component_world( true );
	world->add( miner._Myptr() );	
	world->add( miner1._Myptr() );

	getchar();

	return 0;
}