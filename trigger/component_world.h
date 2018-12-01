#pragma once
#include <list>
#include <memory>
#include <chrono>
#include <thread>
#include <mutex>
#include "../json/single_include/nlohmann/json.hpp"
#include <fstream>

#include "actor.h"


using namespace std;

namespace trigger
{
	class component_world : public trigger::component
	{
		typedef chrono::high_resolution_clock time;
		typedef chrono::time_point<chrono::steady_clock> Time;

	private:
		string name;
		list<component*> components;
		Time start_time;
		chrono::duration<float> delta_time;
		chrono::duration<float> run_time;
		thread main_thread;
		bool use_thread;
		mutex lock;
		float time_scale = 1.0f;

	public:
		//Build a new World
		explicit inline component_world( bool UseThread )
		{
			components = list<component*>();
			start_time = time::now();

			use_thread = UseThread;
			if( UseThread )
			{
				main_thread = thread( &component_world::update, this, delta_time.count() );
			}
		}

		explicit inline component_world( bool UseThread, string name )
		{
			components = list<component*>();
			start_time = time::now();
			set_name( name );

			use_thread = UseThread;
			if( UseThread )
			{
				main_thread = thread( &component_world::update, this, delta_time.count() );
			}
		}

		inline float get_delta_time() const noexcept
		{
			return delta_time.count();
		}

		inline void set_name( const string name )
		{
			this->name = name;
		}
		inline const string& get_name()
		{
			return this->name;
		}

		template<typename T>
		inline constexpr T* get() const
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

		inline list<component*> get_all() const
		{
			return this->components;
		}

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
			if( target != nullptr && components.size() != 0 )
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
					while( this->active )
					{
						update_all();
					}
				}
			}
		}

		void update_all()
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
							i->update( this->delta_time.count() * time_scale * i->time_scale );
						}
					}
				}
				lock.unlock();
				delta_time = chrono::duration_cast<chrono::duration<float>>(time::now() - t);
			}
		}
		
		//TODO
		static bool save_world( string path, string name, component_world *world )
		{
			mutex lockd;
			using json = nlohmann::json;

			lockd.lock();

			thread t( []( string p, string n, component_world *w )
			{
				ofstream o( p + "/" + n );
				if( !o.is_open() ) return false;
				json j;
				j["world"]["name"] = w->name;
				j["world"][w->name]["use_thread"] = w->use_thread;
				j["world"][w->name]["components"] = {};
				auto ac = w->get_components<actor>();
				for( auto i : ac )
				{
					json actors;
					auto trans = i->s_transform;
					actors["name"] = i->name;
					//actors[i->name]["info"] = {{"active", i->active},{"child", i->child->name },{"parent", i->parent->name },{ "static" , i->is_static}};
					actors[i->name]["trans"]["pos"] = {trans.position.x, trans.position.y,trans.position.z,trans.position.w};
					actors[i->name]["trans"]["rot"] = {trans.rotation.x, trans.rotation.y,trans.rotation.z,trans.rotation.w};
					actors[i->name]["trans"]["sca"] = {trans.scale.x, trans.scale.y,trans.scale.z,trans.scale.w};
					actors[i->name]["active"] = i->active;
					if( i->child != nullptr )
						actors[i->name]["child:" + i->child->name] = i->child->name;
					if( i->parent != nullptr )
						actors[i->name][i->parent->name] = i->parent->name;
					actors[i->name]["static"] = i->is_static;
					j["world"][w->name]["components"]["actors"] += actors;
				}
				
				o << j;
				o.close();
			}, path, name, world
			);
			t.join();
			lockd.unlock();
			return true;
		}
		
		//TODO
		static inline component_world* load_world( string path )
		{
			using json = nlohmann::json;

			bool use_thread_local = false;
			json j;

			std::ifstream i( path.c_str() );
			if( !i.is_open() ) return nullptr;
			i >> j;
			string world_name = j["world"]["name"].get<string>();
			auto world = new component_world( j["world"][world_name]["use_thread"].get<bool>(), world_name );
			auto actors = j["world"][world_name]["components"]["actors"];

			return world;
		}

		~component_world()
		{
			components.clear();
			main_thread.join();
		}
	};
}
