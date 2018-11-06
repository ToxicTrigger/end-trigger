#pragma once
#include <cmath>
#include "../json/single_include/nlohmann/json.hpp"

namespace trigger
{
#define eps (1e-6)
	class vec
	{
	private:
		bool _static;

	public:
		//Why 4 parm ?
		//				 used color. 
		//	  r, g, b, a
		float x, y, z, w;

		static const vec X;
		static const vec Y;
		static const vec Z;
		static const vec ZERO;
		static const vec ONE;
		static const float pi;

		// when w == 0 ? that vector is static vector.
		explicit inline vec(float x = 0, float y = 0, float z = 0, float w = 0)
		{
			this->x = x;
			this->y = y;
			this->z = z;
			this->w = w;
		};

		inline bool is_static() const noexcept
		{
			return w == 0;
		}

		static inline float rad(float deg) { return deg * pi / 180; }
		static inline float deg(float rad) { return rad * 180 / pi; }

		vec operator -()const { return vec(-x, -y, -z); }
		friend vec operator +(const vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				return  vec(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator -(const vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				return  vec(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator -(const float p, const vec& v2)
		{
			if (v1.w != 0)
				return  vec(p - v2.x, p - v2.y, p - v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend void operator +=(vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				v1 = vec(v1.x + v2.x, v1.y + v2.y, v1.z + v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend void operator -=(vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				v1 = vec(v1.x - v2.x, v1.y - v2.y, v1.z - v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator /(const vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				return  vec(v1.x / v2.x, v1.y / v2.y, v1.z / v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator *(const vec& v1, const vec& v2)
		{
			if (v1.w != 0)
				return  vec(v1.x*v2.x, v1.y*v2.y, v1.z*v2.z);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator /(const vec& v1, const float v)
		{
			if (v1.w != 0)
				return  vec(v1.x / v, v1.y / v, v1.z / v);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator *(const vec& v1, const float v)
		{
			if (v1.w != 0)
				return  vec(v1.x*v, v1.y*v, v1.z*v);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend vec operator *(const float v, const vec& v1)
		{
			if (v1.w != 0)
				return  vec(v1.x*v, v1.y*v, v1.z*v);
			else
				return  vec(v1.x, v1.y, v1.z);
		}
		friend bool operator ==(const vec& v1, const vec& v2)
		{
			return  std::abs(v1.x - v2.x) < eps && std::abs(v1.y - v2.y) < eps && std::abs(v1.z - v2.z) < eps;
		}
		friend bool operator !=(const vec& v1, const vec& v2)
		{
			return  !(v1 == v2);
		}

		vec& operator =(const vec& v)
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = v.w;
			return *this;
		}

		nlohmann::json to_json()
		{
			nlohmann::json j;
			j["x"] = x;
			j["y"] = y;
			j["z"] = z;
			j["w"] = w;
			return j;
		}

		static vec parse_json(const nlohmann::json v)
		{
			return vec(v["x"], v["y"], v["z"], v["w"]);
		}

		vec inverse()const { return -(*this); }
		float sum()const { return std::abs(x) + std::abs(y) + std::abs(z); }
		float length()const { return std::sqrt(x*x + y * y + z * z); }
		float lengthSquared()const { return x * x + y * y + z * z; }

		inline vec pow2(const vec& v)
		{
			return vec(v.x*v.x, v.y*v.y, v.z*v.z);
		}
		inline vec pow(const vec& v, float p)
		{
			return vec(std::pow(v.x, p), std::pow(v.y, p), std::pow(v.z, p));
		}
		inline vec abs(const vec& v)
		{
			return vec(std::abs(v.x), std::abs(v.y), std::abs(v.z));
		}

		bool is_same_direction(const vec& v)
		{
			auto d = dot(v);
			return  std::abs(d - 1) < eps;
		}
		bool is_opposite(const vec& v)
		{
			auto d = dot(v);
			return  std::abs(d + 1) < eps;
		}
		float dot(const vec& v)const
		{
			return x * v.x + y * v.y + z * v.z;
		}

		vec cross(const vec& v)const
		{
			return vec(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
		}

		vec normalized() const
		{
			auto ls = lengthSquared();
			if (std::abs(ls) < eps || std::abs(ls - 1) < eps)
			{
				return (*this);
			}
			return (*this) / sqrt(ls);
		}

		void cutoff(vec& v)
		{
			if (std::abs(x) < v.x) { x = 0; }
			if (std::abs(y) < v.y) { y = 0; }
			if (std::abs(z) < v.z) { z = 0; }
		}

		float angle(const vec& with = vec::X)const
		{
			return acos(dot(with) / length() / with.length());
		}

		vec interpolated(vec& to, float by)
		{
			return (*this) + (to - (*this)) * by;
		}


		vec orthogonal()const
		{
			float xx = std::abs(x);
			float yy = std::abs(y);
			float zz = std::abs(z);

			vec other = xx < yy ? (xx < zz ? vec::X : vec::Z) : (yy < zz ? vec::Y : vec::Z);
			return (*this).cross(other);
		}

	};
}