#ifndef SGFX_PRIMITIVE_TYPES_H
#define SGFX_PRIMITIVE_TYPES_H

#include <cstdint>

namespace sgfx
{
	struct vec
	{
		int x, y;

    vec operator * (const int scale) const
    {
      return vec { scale*x, scale*y };
    }
    friend vec operator * (const int scale, const vec& v)
    {
      return vec { scale*v.x, scale*v.y };
    }
  };

	struct point
	{
    int x, y;

    vec operator - (const point& p) const
    {
      return vec {x-p.x, y-p.y};
    }
    point operator + (const vec& v) const
    {
      return point {x+v.x, y+v.y};
    }
    point operator - (const vec& v) const
    {
      return point {x-v.x, y-v.y};
    }
    point operator += (const vec& v)
    {
      x += v.x;
      y += v.y;
      return *this;
    }
    bool operator == (const point& p) const
    {
      return (x == p.x && y == p.y);
    }
	};
	
	struct dimension
	{
		std::uint16_t w, h;
	};
	
	struct rectangle
	{
		point top_left;
		dimension size;
	};
}

#endif
