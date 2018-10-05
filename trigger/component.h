#pragma once

class component
{
public:
	bool active = true;
	virtual void update(float delta) noexcept = 0;
};