#pragma once
#include <string>
#include <DirectXMath.h>
#include "component.h"
#include "fsm.h"
#include "vec.h"
using namespace DirectX;

namespace trigger
{
	struct transform
	{
		vec position, rotation, scale;
	};

	class actor : public trigger::component
	{
	public:
		std::string name;
		std::unique_ptr<actor> parent;
		std::unique_ptr<actor> child;
		transform s_transform;
		trigger::fsm::map fsm;
		std::list<component> s_components;
		
		void update( float delta ) noexcept
		{
			fsm.update( delta );
		}

		actor()
		{
			name = "Actor";
			s_transform.position = vec( 0, 0, 0 );
			s_transform.rotation = vec( 0, 0, 0 );
			s_transform.scale = vec( 1, 1, 1 );
			s_components = std::list<component>();
		}

		~actor()
		{

		}
	};
}


