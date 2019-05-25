#include <sgfx/image.hpp>

#include <stdexcept>

#include <limits>
#include <string>

#include <fstream>
#include <sstream>
#include <iostream>

#include <ctime>
#include <chrono>
#include <iomanip>

using namespace sgfx;

canvas sgfx::load_ppm(const std::string& path)
{
  std::ifstream file(path);
  if (!file.is_open())
    throw std::runtime_error({"Fehler beim Laden von " + path});

  std::string ppm_header;
  int img_w;
  int img_h;
  int max_col_val;

  file >> ppm_header;
  if (ppm_header != "P3")
    throw std::runtime_error({"Keine ppm Datei: " + path});
  file.ignore(1); // ignore newline

  if (file.peek() == '#')
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  // read size
  file >> img_w >> img_h >> max_col_val;

  // fill canvas
  canvas c {{std::uint16_t(img_w), std::uint16_t(img_h)}};
  for (auto i=0; i<img_w*img_h; i++) {
    int r, g, b;
    file >> r >> g >> b;
    color::rgb_color col { std::uint8_t(r), std::uint8_t(g), std::uint8_t(b) };
    c.pixels()[i] = col;
  }

	return c;
}


void sgfx::save_ppm(canvas_view img, const std::string& filename)
{
  size_t ext_pos = filename.rfind(".");
  std::string filename_ts { filename };
  std::stringstream ts_ss;
  auto now_clock = std::chrono::system_clock::now();
  auto now_time = std::chrono::system_clock::to_time_t(now_clock);
  ts_ss << std::put_time(std::localtime(&now_time), "_%F_%T");
  filename_ts.insert(ext_pos, ts_ss.str());

  // log
  std::cout << "Saving Screenshot: " << filename_ts << std::endl;
  std::ofstream ofile(filename_ts);

  // write header
  ofile << "P3\n" << img.width() << "\n" << img.height() << "\n255\n";

  // write pixels
  for (auto i=0; i<img.width()*img.height(); i++) {
    int r = img.pixels()[i].red();
    int g = img.pixels()[i].green();
    int b = img.pixels()[i].blue();
    ofile << r << " " << g << " " << b << "\n";
  }
}

rle_image sgfx::load_rle(const std::string& filename)
{
  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error({"Fehler beim Laden von " + filename});

  std::uint16_t w;
  file.read(reinterpret_cast<char*>(&w), sizeof(w));

  std::uint16_t h;
  file.read(reinterpret_cast<char*>(&h), sizeof(h));

  rle_image img {w, h, {}};
  std::uint16_t runs_per_line = 0;
  while (!file.eof()) {
    if (runs_per_line == 0) {
      file.read(reinterpret_cast<char*>(&runs_per_line), sizeof(runs_per_line));
      if (runs_per_line == 0)
        break;
    }
    std::uint8_t run_len;
    file.read(reinterpret_cast<char*>(&run_len), sizeof(run_len));
    std::uint8_t r;
    file.read(reinterpret_cast<char*>(&r), sizeof(r));
    std::uint8_t g;
    file.read(reinterpret_cast<char*>(&g), sizeof(g));
    std::uint8_t b;
    file.read(reinterpret_cast<char*>(&b), sizeof(b));
    img.runs.push_back({runs_per_line, run_len, {r, g, b}});
    runs_per_line--;
  }

	return img;
}

void sgfx::save_rle(const rle_image& src, const std::string& filename)
{
  std::ofstream file(filename, std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error({"Fehler beim Speichern von " + filename});

  std::uint16_t w = src.w;
  file.write(reinterpret_cast<char*>(&w), sizeof(w));

  std::uint16_t h = src.h;
  file.write(reinterpret_cast<char*>(&h), sizeof(h));

  std::uint16_t runs_per_line = 0;
  for (auto const& run : src.runs) {
    if (runs_per_line == 0) {
      runs_per_line = run.dist;
      file.write(reinterpret_cast<char*>(&runs_per_line), sizeof(runs_per_line));
    }
    std::uint8_t run_len = run.len;
    file.write(reinterpret_cast<char*>(&run_len), sizeof(run_len));
    std::uint8_t r = run.col.red();
    file.write(reinterpret_cast<char*>(&r), sizeof(r));
    std::uint8_t g = run.col.green();
    file.write(reinterpret_cast<char*>(&g), sizeof(g));
    std::uint8_t b = run.col.blue();
    file.write(reinterpret_cast<char*>(&b), sizeof(b));
    runs_per_line--;
  }
}

rle_image sgfx::rle_encode(canvas_view src)
{
  std::uint16_t w = src.width();
  std::uint16_t h = src.height();

  rle_image img {w, h, {}};
  std::uint16_t runs_per_line = 0;
  auto i = 0;
  while (i < w*h) {

    if (i%w == 0) {
      runs_per_line = 0;
    }

    // create run
    std::uint8_t run_len = 0;
    color::rgb_color col { src.pixels()[i] };
    while ((++i < w*h) && col == src.pixels()[i] && run_len++ <= 255);
    img.runs.insert(img.runs.begin(), {++runs_per_line, run_len, col});
  }

	return img;
}

auto index_(std::uint16_t x, std::uint16_t y, std::uint16_t w) {
  return x + y*w;
}

void sgfx::draw(canvas_view target, const rle_image& source, point top_left)
{
  std::uint16_t w = target.width();
  std::uint16_t h = target.height();

  auto x = 0;
  auto y = 0;
  for (auto const& run : source.runs) {

    // draw run into canvas
    for (auto j = 0; j<run.len; j++) {
      auto i = index_(top_left.x+x+j, top_left.y+y, w);
      if (i > w*h-1) return;
      target.pixels()[i] = run.col;
    }
    x += run.len;
    if (run.dist == 1) {
      x = 0;
      y++;
    }
  }
}

void sgfx::draw(canvas_view target, const rle_image& source, point top_left, color::rgb_color colorkey)
{
  std::uint16_t w = target.width();
  std::uint16_t h = target.height();

  auto x = 0;
  auto y = 0;
  for (auto const& run : source.runs) {

    // draw run into canvas
    for (auto j = 0; j<run.len; j++) {
      auto i = index_(top_left.x+x+j, top_left.y+y, w);
      if (i > w*h-1) return;
      if (run.col == colorkey) break;
      target.pixels()[i] = run.col;
    }
    x += run.len;
    if (run.dist == 1) {
      x = 0;
      y++;
    }
  }
}
