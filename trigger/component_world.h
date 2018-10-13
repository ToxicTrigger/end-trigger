#pragma once
#include <list>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>

#include "component.h"

using namespace std;

namespace trigger
{
	class component_world : public trigger::component
	{
		typedef chrono::high_resolution_clock time;
		typedef chrono::time_point<chrono::steady_clock> Time;

	private:
		list<component*> components;
		Time start_time;
		chrono::duration<float> delta_time;
		chrono::duration<float> run_time;
		thread main_thread;
		bool use_thread;
		mutex lock;

	public:
		//Build a new World
		inline component_world( bool UseThread )
		{
			components = list<component*>();
			start_time = time::now();

			use_thread = UseThread;
			if( UseThread )
			{
				main_thread = thread( &component_world::update, this, delta_time.count() );
			}
		}

		inline float get_delta_time() noexcept
		{
			return delta_time.count();
		}

		template<typename T>
		inline constexpr T* get()
		{
			for( auto i : components )
			{
				auto t = dynamic_cast<T*>(i);
				if( t != nullptr )
				{
					return t;
				}
			}
			return nullptr;
		};

		template<typename T>
		inline constexpr list<T*> get_components()
		{
			list<T*> tmp = list<T*>();
			for( auto i : components )
			{
				auto t = dynamic_cast<T*>(i);
				if( t != nullptr )
				{
					tmp.push_back( t );
				}
			}
			return tmp;
		};

		inline component* get( unsigned int index ) noexcept
		{
			if( index >= components.size() ) return nullptr;

			auto i = components.begin();
			std::advance( i, index );
			return *i;
		}
		
		inline constexpr bool delete_component( component *target ) noexcept
		{
			if( target != nullptr && components.size() != 0)
			{
				lock.lock();
				components.remove( target );
				lock.unlock();
				return true;
			}
			return false;
		}

		//add component in world-component-list
		inline constexpr void add( component * com ) noexcept
		{
			if( com != nullptr ) components.push_back( com );
		}

		inline void clean_component() noexcept
		{
			if( components.size() != 0 )
			{
				auto delete_list = std::list<component*>();
				for( auto i : components )
				{
					if( !i->active ) delete_list.push_back( i );
				}

				for( auto i : delete_list )
				{
					components.remove( i );
				}
			}
		}

		//simulating world
		inline void update( float delta ) noexcept
		{
			while( use_thread )
			{
				if( components.size() != 0 )
				{
					while( this->active && use_thread )
					{
						update_all();
					}
				}
			}
		}

		inline void update_all()
		{
			if( components.size() != 0 )
			{
				run_time = chrono::duration_cast<chrono::duration<float>>(time::now() - start_time);
				auto t = time::now();
				lock.lock();
				for( auto i : components )
				{
					if( i != nullptr )
					{
						if( i->active )
						{
							i->update( this->delta_time.count() );
						}
					}
				}
				lock.unlock();
				delta_time = chrono::duration_cast<chrono::duration<float>>(time::now() - t);
			}
		}

		~component_world()
		{
			components.clear();
			main_thread.join();
		}
	};
}
