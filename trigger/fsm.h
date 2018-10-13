#pragma once
#include <thread>
#include <string>
#include <list>
#include "component.h"
#include <iostream>
#include <memory>

namespace trigger
{
	namespace fsm
	{
		class state
		{
			std::string name;
		public:
			explicit inline state( std::string name = "Unknown" ) noexcept
			{
				this->name = name;
			}

			inline std::string const get_name() const noexcept
			{
				return this->name;
			}
			virtual void begin_state()
			{};
			virtual void end_state()
			{};
			virtual void update( float delta )
			{};
		};

		class link
		{
		private:
			std::unique_ptr<state> cur;
			std::unique_ptr<state> next;
			int ops;

		public:
			explicit inline link() noexcept
			{
				cur = nullptr;
				next = nullptr;
				ops = 1;
			}

			explicit inline link( state *current, state *next ) : link()
			{
				this->cur._Myptr() = current;
				this->next._Myptr() = next;
			}

			inline const state* const get_current_state() const noexcept
			{
				return this->cur.get();
			};
			inline const state* const get_next_state() const noexcept
			{
				return this->next.get();
			};
			inline constexpr int const get_ops() const noexcept
			{
				return this->ops;
			};
			inline constexpr void set_ops( int op ) noexcept
			{
				ops = op;
			};
		};

		class map : public component
		{
		private:
			std::unique_ptr<state> now_state;
			std::list<state*> states;
			std::list<link*> links;

			inline void simulate( float delta ) noexcept
			{
				now_state->update( delta );
				for( auto i : this->links )
				{
					if( i->get_current_state()->get_name() == now_state->get_name() )
					{
						// ops == 0 , move now_state between link
						if( i->get_ops() == 0 )
						{
							i->set_ops( i->get_ops() + 1 );
							now_state->end_state();
							now_state._Myptr() = const_cast<state*>(i->get_next_state());
							now_state->begin_state();
							return;
						}
					}
				}
			}

		public:
			inline map()
			{
				states = std::list<state*>();
				links = std::list<link*>();

				auto idle = new state( "idle" );
				add_state( idle );
				now_state = std::make_unique<state>( idle->get_name() );
			}

			inline map( state *def_state ) : map()
			{
				// inited state idle
				add_state( def_state );
				link *def = new link( now_state.get(), def_state );
				def->set_ops( 0 );
				links.push_back( def );
			}

			inline state* const get_state( std::string name ) const noexcept
			{
				for( auto i : states )
				{
					if( i->get_name() == name )
					{
						return i;
					}
				}
				return nullptr;
			}

			inline state* get_state( state *state ) const noexcept
			{
				for( auto i : states )
				{
					if( i == state ) return i;
				}
				return nullptr;
			}

			inline bool link_state( std::string state1, std::string state2 ) noexcept
			{
				state *a = get_state( state1 );
				state *b = get_state( state2 );
				if( a == nullptr || b == nullptr )
				{
					return false;
				}
				link *tmp = new link( a, b );
				this->links.push_back( tmp );
				return true;
			}

			inline const state* const get_now_state() const noexcept
			{
				return now_state.get();
			}

			inline constexpr void add_state( state *new_state ) noexcept
			{
				if( new_state != nullptr )
				{
					states.push_back( new_state );
				}
			}

			inline const void add_state( std::string state_name ) noexcept
			{
				state *tmp = new state( state_name );
				states.push_back( tmp );
			}

			inline bool delete_state( state * state ) noexcept
			{
				if( state != nullptr )
				{
					states.remove( state );
					return true;
				}
				return false;
			}

			inline bool delete_state( std::string name ) noexcept
			{
				if( delete_state( get_state( name ) ) )
				{
					return true;
				}
				return false;
			}

			inline const link* const get_link( std::string state1, std::string state2 ) const noexcept
			{
				state *tmp = get_state( state1 );
				state *tmp2 = get_state( state2 );
				if( tmp != nullptr && tmp2 != nullptr )
				{
					for( auto i : links )
					{
						if( i->get_current_state()->get_name() == tmp->get_name() && i->get_current_state()->get_name() == tmp2->get_name() )
						{
							return i;
						}
					}
				}
				return nullptr;
			}

			inline bool delete_link( std::string state1, std::string state2 ) noexcept
			{
				auto t = get_link( state1, state2 );
				if( t != nullptr )
				{
					links.remove( const_cast<link*>(t) );
					return true;
				}
				return false;
			}

			inline bool change_link( std::string state1, std::string state2, unsigned int op ) const noexcept
			{
				state *tmp = get_state( state1 );
				state *tmp2 = get_state( state2 );
				if( tmp != nullptr && tmp2 != nullptr )
				{
					for( auto i : links )
					{
						if( i->get_current_state()->get_name() == tmp->get_name()
							&& i->get_next_state()->get_name() == tmp2->get_name() )
						{
							i->set_ops( 0 );
						}
					}
					return true;
				}
				return false;
			}

			inline void update( float delta ) noexcept
			{
				simulate( delta );
			}

			~map()
			{
				if( now_state != nullptr )
				{
					now_state.release();
				}

				states.clear();
				links.clear();
			}

		};
	}
}


