#pragma once
#include <thread>
#include <string>
#include <list>
#include "component.h"
#include <iostream>
#include <memory>

/*
	간단한 FSM 구현
	사용법 :
		1. fsm 객체를 생성한다.
		2. fsm::add_state() 로 state 를 추가 해준다.
		3. fsm::link_state() 로 두 state 를 이어주는 링크를 만든다.
		4. fsm::simulate() 를 호출하여 now_state 를 갱신한다.

	알아두어야 하는 것:
		각 링크의 ops 가 0 일 때 다음 상태로 전이한다.
		이 ops 를 제어하고 싶을 땐 fsm::change_link() 를 호출한다.
		해당 함수는 두 state 를 이어주는 link 의 ops 를 제어할 수 있도록 한다.

*/
namespace trigger
{
	namespace fsm
	{
		class state
		{
		public:
			std::string name;

			inline state( std::string name = "Unknown" ) noexcept
			{
				this->name = name;
			}

			//상태 진입시 호출됩니다.
			virtual void begin_state()
			{};
			//상태 변경시 호출 됩니다.
			virtual void end_state()
			{};
			//현재 상태일 때 수시로 호출됩니다.
			virtual void update( float delta )
			{};
		};

		class link
		{
		public:
			std::unique_ptr<state> cur;
			std::unique_ptr<state> next;
			int ops;

			inline link() noexcept
			{
				cur = nullptr;
				next = nullptr;
				ops = 1;
			}

			inline link( state *current, state *next ) : link()
			{
				this->cur._Myptr() = current;
				this->next._Myptr() = next;
			}

		};

		class map : public component
		{
		private:
			std::unique_ptr<state> now_state;
			std::list<state*> states;
			std::list<link*> links;

		public:
			inline map()
			{
				states = std::list<state*>();
				links = std::list<link*>();

				auto idle = new state( "idle" );
				add_state( idle );
				now_state = std::make_unique<state>(idle->name);
			}

			inline map( state *def_state ) : map()
			{
				// inited state idle
				add_state( def_state );
				link *def = new link( now_state.get(), def_state );
				def->ops = 0;
				links.push_back( def );
			}

			inline state* get_state( std::string name ) const noexcept
			{
				for( auto i : states )
				{
					if( i->name == name )
					{
						return i;
					}
				}
				return nullptr;
			}

			inline state* get_state( state *state ) const
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

			inline const state* get_now_state() const
			{
				return now_state.get();
			}

			inline void add_state( state *new_state ) noexcept
			{
				if( new_state != nullptr )
				{
					states.push_back( new_state );
				}
			}

			inline void add_state( std::string state_name ) noexcept
			{
				state *tmp = new state( state_name );
				states.push_back( tmp );
			}

			inline bool delete_state( state * state ) noexcept
			{
				if( state != nullptr )
				{
					states.remove( state );
				}
			}

			inline bool delete_state( std::string name ) noexcept
			{
				if( delete_state( get_state( name )) )
				{
					return true;
				}
				return false;
			}

			inline link* get_link( std::string state1, std::string state2 )
			{
				state *tmp = get_state( state1 );
				state *tmp2 = get_state( state2 );
				if( tmp != nullptr && tmp2 != nullptr )
				{
					for( auto i : links )
					{
						if( i->cur->name == tmp->name && i->next->name == tmp2->name )
						{
							return i;
						}
					}
				}
				return nullptr;
			}

			inline bool delete_link( std::string state1, std::string state2 ) noexcept
			{
				if( get_link( state1, state2 ) != nullptr )
				{
					return true;
				}
				return false;
			}

			inline void simulate( float delta ) noexcept
			{
				now_state->update( delta );
				for( auto i : this->links )
				{
					if( i->cur->name == now_state->name )
					{
						// ops == 0 , move now_state between link
						if( i->ops == 0 )
						{
							i->ops += 1;
							now_state->end_state();
							now_state._Myptr() = i->next._Myptr();
							now_state->begin_state();
							return;
						}
					}
				}
			}

			inline bool change_link( std::string state1, std::string state2, unsigned int op ) const noexcept
			{
				state *tmp = get_state( state1 );
				state *tmp2 = get_state( state2 );
				if( tmp != nullptr && tmp2 != nullptr )
				{
					for( auto i : links )
					{
						if( i->cur->name == tmp->name && i->next->name == tmp2->name )
						{
							i->ops = op;
						}
					}
					return true;
				}
				return false;
			}

			void update( float delta ) noexcept
			{
				simulate( delta );
			}

			~map()
			{
				if( now_state != nullptr )
				{
					now_state.release();
					//delete now_state;
				}

				states.clear();
				links.clear();
			}

		};
	}
}


