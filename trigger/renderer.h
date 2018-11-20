#pragma once
#include "d3dUtil.h"

class renderer : public trigger::component
{
public:
	GeometryGenerator::MeshData mesh;
	Material material;


	void draw()
	{

	}
};