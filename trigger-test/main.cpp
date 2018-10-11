#include "Miner.h"
#include "../trigger/component_world.h"

using namespace std;

auto main() -> int
{
	Miner *miner = new Miner();
	component_world *world = new component_world( true );
	world->add( miner );
	world->get<Miner>()->active = false;
	world->clean_component();
	getchar();

	return 0;
}