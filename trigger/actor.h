#pragma once
#include <string>
#include <DirectXMath.h>

#include "component.h"
#include "fsm.h"

using namespace DirectX;

namespace trigger
{
	class actor : public trigger::component
	{

	public:
		std::string name;
		XMFLOAT3 position, rotation, scale;
		trigger::fsm::map fsm;

		void update( float delta ) noexcept
		{
			fsm.update( delta );
		}

		actor()
		{
			name = "Actor";
			position = XMFLOAT3( 0, 0, 0 );
			rotation = XMFLOAT3( 0, 0, 0 );
			scale = XMFLOAT3( 1, 1, 1 );
		}

		~actor()
		{

		}
	};
}


