#pragma once

namespace trigger
{
	class component
	{
	public:
		float time_scale = 1.0f;
		bool active = true;
		virtual void update(float delta) noexcept
		{};
	};
}
