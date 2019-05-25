#ifndef SGFX_IMAGE_H
#define SGFX_IMAGE_H

#include <sgfx/canvas.hpp>
#include <sgfx/color.hpp>

#include <string>
#include <vector>
#include <iostream>

namespace sgfx
{
	//would be better to use std::filesystem::path, but support seems to be lacking on some platforms(...) and it seems like not everbody is willing to use the VM xD
	canvas load_ppm(const std::string& path);
	void save_ppm(canvas_view source, const std::string& path);
	
  struct rle_run {
    std::uint16_t dist; // distance to end of line

    std::uint8_t len;
    color::rgb_color col;

    friend std::ostream& operator << (std::ostream& os, const rle_run& run) {
      os << int(run.len) << "(" <<
        int(run.col.red()) << ", " <<
        int(run.col.green()) << ", " <<
        int(run.col.blue()) << ")";
      return os;
    }
  };

	struct rle_image {
    std::uint16_t w;
    std::uint16_t h;

		std::vector<rle_run> runs;

    friend std::ostream& operator << (std::ostream& os, const rle_image& img) {
      os << "rle_image(" << int(img.w) << ", " << int(img.h) << ") {" << std::endl;
      for (auto const& run : img.runs) {
        os << run;
      }
      os << "}" << std::endl;
      return os;
    }
  };

	rle_image load_rle(const std::string& path);
	void save_rle(const rle_image& source, const std::string& path);
	rle_image rle_encode(canvas_view source);
	void draw(canvas_view target, const rle_image& source, point top_left);
	void draw(canvas_view target, const rle_image& source, point top_left, color::rgb_color colorkey);
}

#endif
