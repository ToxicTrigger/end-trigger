#pragma once
#include <string>
#include <DirectXMath.h>

#include "component.h"

using namespace DirectX;

class actor : public trigger::component
{

public:
	XMVECTOR position, rotation, scale;

	void update( float delta ) noexcept
	{

	}

	actor()
	{

	}
	~actor()
	{

	}
};

