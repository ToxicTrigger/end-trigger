#include "Miner.h"
#include "../trigger/component_world.h"

using namespace std;

auto main() -> int
{
	auto start = std::chrono::high_resolution_clock::now();
	auto map = new trigger::fsm::map();

	start = std::chrono::high_resolution_clock::now();
	for( int i = 0; i < 10000; ++i )
	{
		map->add_state( "Hello" );
	}
	cout << (std::chrono::high_resolution_clock::now() - start).count() << endl;
	getchar();

	return 0;
}