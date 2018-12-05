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
		bool is_static;
		std::string name;
		std::unique_ptr<actor> parent;
		std::unique_ptr<actor> child;
		transform s_transform;
		trigger::fsm::map fsm;
		std::list<component> s_components;
		
		void update( float delta ) noexcept override
		{
			fsm.update( delta );
		}

		actor() : component()
		{
			s_transform.position = vec( 0, 0, 0 );
			s_transform.rotation = vec( 0, 0, 0 );
			s_transform.scale = vec( 1, 1, 1 );

			//when that macro call in construtor, it working Initialized variable.
			//so when new actor born and wake up, his name r "Actor".
			SAVE_VAR_INIT(name, name.c_str());
			SAVE_VAR(s_transform.position.to_json());
			SAVE_VAR(s_transform.rotation.to_json());
			SAVE_VAR(s_transform.scale.to_json());

			s_components = std::list<component>();
		}

		~actor()
		{

		}
	};
}


