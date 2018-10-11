#include "Miner.h"
#include "../trigger/component_world.h"

using namespace std;

auto main() -> int
{
	auto miner = make_unique<Miner>();
	miner->name = "1";
	auto miner1 = make_unique<Miner>();
	miner1->name = "2";
	component_world *world = new component_world( true );
	world->add( miner.get() );	
	world->add( miner1.get() );
	world->delete_component( miner.get() );
	getchar();

	return 0;
}