#include <string>
#include <vector>
#include <unordered_map>

#include <sstream>
#include <fstream>
#include <iostream>

#include <ctime>
#include <cmath>
#include <chrono>
#include <iomanip>

#include <bitset>
#include <algorithm>

//#define NDEBUG

std::string append_timestamp(const char* _file) {

  std::stringstream ts_ss;
  auto now_clock = std::chrono::system_clock::now();
  auto now_time = std::chrono::system_clock::to_time_t(now_clock);
  ts_ss << std::put_time(std::localtime(&now_time), "_%F_%T");

  std::string file {_file};
  file.append(ts_ss.str());
  return file;
}

class node {
  public:
    int byte;
    int count;

    node* left;
    node* right;

    node(int _byte, int _count):
      byte(_byte), count(_count), left(nullptr), right(nullptr) {}

    node(int _byte, int _count, node* _left, node* _right):
      byte(_byte), count(_count), left(_left), right(_right) {}

    ~node() {
    }

    void print() const {
      if (byte > 31 && byte < 127)
        std::cout << char(byte);
      else 
        std::cout << int(byte);
      std::cout << "(" << count << ")";
    }

    void huffcodes(int codes[256], int bitc[256], int bc, int code) const {

      if (byte != -1) {
        codes[byte] = code;
        bitc[byte] = bc;
        return;
      }

      if (left)
        left->huffcodes(codes, bitc, bc+1, ((code << 1) | 0));

      if (right)
        right->huffcodes(codes, bitc, bc+1, ((code << 1) | 1));
    }
};

void huffman_encode(const char* cfile) {

  std::ifstream file(cfile, std::ios::binary); 
  if (!file.is_open()) {
    std::cerr << "Error loading input file" << std::endl;
    exit(1);
  }

  // count byte types
  int byte_type_count[256];
  std::fill_n(byte_type_count, 256, 0);

  auto char_to_byte = [](char c) -> int { return c+128; };
  //auto byte_to_char = [](int c) -> char { return char(c-128); };
  char c; // [-128, 127]
  while (!file.eof()) {
    file.read(&c, sizeof(char));
    byte_type_count[char_to_byte(c)]++;
  }

  // create nodes
  std::vector<node> nodes;

#ifndef NDEBUG
  auto print_nodes = [&nodes]() {
    std::cout << "Nodes: ";
    for (auto& n : nodes) {
      n.print();
      std::cout << " ";
    }
    std::cout << std::endl;
  };
#endif

  for (auto i=0; i<256; i++) {
    int count = byte_type_count[i];
    nodes.push_back({i, count});
  }

  auto node_cmp = [](const node &n1, const node &n2) -> bool {
    return n1.count < n2.count;
  };
  while (nodes.size() > 1) {

#ifndef NDEBUG
    print_nodes();
#endif

    // get minimum 1
    auto min1_it = std::min_element(nodes.begin(), nodes.end(), node_cmp);
    auto *min1 = new node(min1_it->byte, min1_it->count, min1_it->left, min1_it->right);
    nodes.erase(min1_it);

    // get minimum 2
    auto min2_it = std::min_element(nodes.begin(), nodes.end(), node_cmp);
    auto *min2 = new node(min2_it->byte, min2_it->count, min2_it->left, min2_it->right);
    nodes.erase(min2_it);

    // create sum node and sort child nodes
    node sum(-1, min1->count + min2->count);
    sum.left = ((min1->count <= min2->count) ? min1 : min2);
    sum.right = ((min1->count > min2->count) ? min1 : min2);

    // push sum node
    nodes.insert(nodes.begin(), sum);
  }

  // reuse array byte_type_count for huff codes
  int codes[256];
  std::fill_n(codes, 256, -1);
  int bitc[256];
  std::fill_n(bitc, 256, 0);

  node* huff_tree_root = new node(
      nodes[0].byte,
      nodes[0].count,
      nodes[0].left,
      nodes[0].right
      );
  huff_tree_root->huffcodes(codes, bitc, 0, 0);

#ifndef NDEBUG
  std::cout << "Huff Codes: " << std::endl;
  for (auto i=0; i<256; i++) {
    int code = codes[i];
    //if (code == -1) continue;
    std::bitset<8> c(code);
    std::cout << char(i) << ":\t" << c << " - " << bitc[i] << std::endl;
  }
#endif

  return;
  // write file
  std::string ofile_name = append_timestamp(cfile);
  ofile_name.append(".huff");
  std::ofstream ofile(ofile_name, std::ios::binary);
  //
  // file size
  file.clear();
  file.seekg(0, file.end);
  std::uint64_t file_size = file.tellg();
  std::cout << "File size: " << file_size << std::endl;
  ofile.write(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  //
  // codes per byte
  for (auto i=-128; i<128; i++) {
    if (codes[char_to_byte(i)] == -1) {
      //std::uint16_t z = 0; // need 0 bits 
      //ofile.write(reinterpret_cast<char*>(&z), sizeof(z));
      //char c = 0;
      //ofile.write(&c, sizeof(char));
      continue;
    }

    // used bits
    auto used_bits = std::uint16_t(bitc[char_to_byte(i)]);
    ofile.write(reinterpret_cast<char*>(&used_bits), sizeof(used_bits));

    // code
    auto need_bytes = (used_bits%8 == 0 ? used_bits/8 : used_bits/8+1);
    char* code = reinterpret_cast<char*>(&codes[char_to_byte(i)]);
    std::cout << "Byte " << i <<
      ", used bits " << used_bits <<
      ", code " << codes[i] << std::endl;
    for (auto c=0; c<need_bytes; c++)
      ofile.write(&code[need_bytes-1-c], sizeof(char));
  }
  // 
  // rewind input file
  file.clear();
  file.seekg(0, file.beg);
  std::cout << "writing .huff file " << std::endl;
  //
  // encode
  while (!file.eof()) {
    file.read(&c, sizeof(char));
    std::cout << "code index: " << char_to_byte(c) << " ";
    auto code = codes[int(c)];
    auto bytes_to_write = (bitc[code]%8 == 0 ? bitc[code]/8 : bitc[code]/8+1);
    char* cb = reinterpret_cast<char*>(&code);
    std::cout << "encoding " << c << " → " << code << std::endl;
    for (auto c=0; c<bytes_to_write; c++)
      ofile.write(&cb[bytes_to_write-1-c], sizeof(char));
  }
}

void huffman_decode(const char* cfile) {

  std::ifstream file(cfile, std::ios::binary); 
  if (!file.is_open()) {
    std::cerr << "Error loading input file" << std::endl;
    exit(1);
  }

  std::uint64_t file_size;
  file.read(reinterpret_cast<char*>(&file_size), sizeof(file_size));
  std::cout << file_size << std::endl;

  int bitc[256];
  std::unordered_map<std::string, int> code_map;
  for (auto byte=0; byte<256; byte++) {
    std::uint16_t bits;
    file.read(reinterpret_cast<char*>(&bits), sizeof(bits));
    bitc[byte] = bits;
    std::stringstream code;
    auto bits_read = 0;
    while (bits_read < bits) {
      std::uint8_t code_part;
      file.read(reinterpret_cast<char*>(&code_part), sizeof(code_part));
      code << std::bitset<8>(code_part); // bitset strings in lendian
      bits_read += 8;
    }
    code_map[code.str()] = byte;

    std::cout << "byte: ";
    if (byte > 31 && byte < 127)
      std::cout << char(byte);
    else
      std::cout << int(byte);
    std::cout << " bits: " << bits << " \tcode: " << code.str() << std::endl;
  }
  bitc[0] = bitc[0];

  while (!file.eof()) {
    std::stringstream code;
    while (!file.eof() && code_map.count(code.str()) == 0) {
      char c;
      file.read(&c, sizeof(char));
      int select_bits = (int) std::pow(2, bitc[int(c)]-1);
      std::cout << "[" << select_bits << "]";
      code << std::bitset<8>(int(c) & select_bits);
    }
    std::cout << (char)code_map[code.str()] << "(" << code.str() << ")";
  }
  std::cout << std::endl;
}

void rle_encode(const char* file) {

  // read input ppm file
  std::ifstream ifile(file);
  if (!ifile.is_open())
    throw std::runtime_error("Fehler beim Lesen von Eingabedatei " + std::string(file));

  std::string ppm_header;
  int img_w;
  int img_h;
  int max_col_val;

  ifile >> ppm_header;
  if (ppm_header != "P3")
    throw std::runtime_error({"Keine ppm Datei: " + std::string(file)});
  ifile.ignore(1); // ignore newline

  if (ifile.peek() == '#') // ignore comment after header
    ifile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  // read size, max. color value
  ifile >> img_w >> img_h >> max_col_val;

  //
  // write rle output file
  std::string ofilename = append_timestamp(file);
  ofilename.append(".rle");
  std::ofstream ofile(ofilename, std::ios::binary);
  if (!ofile.is_open())
    throw std::runtime_error({"Fehler beim Schreiben von Ausgabedatei " + ofilename});

  std::uint16_t w = img_w;
  ofile.write(reinterpret_cast<char*>(&w), sizeof(w));

  std::uint16_t h = img_h;
  ofile.write(reinterpret_cast<char*>(&h), sizeof(h));

  struct rle_run {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t len;
  };
  std::vector<rle_run> rle_row;

  int run_r;
  int run_g;
  int run_b;
  int run_len = 1;
  int color_count = 0;
  ifile >> run_r >> run_g >> run_b;
  while (!ifile.eof()) {

    // read color
    int r, g, b;
    ifile >> r >> g >> b;
    color_count++;

    if (run_r != r || run_g != g || run_b != b ||
        (color_count % img_w == 0)) { // new run
      rle_row.push_back({
          std::uint8_t(run_r),
          std::uint8_t(run_g),
          std::uint8_t(run_b),
          std::uint8_t(run_len)
          });
      run_len = 1;
      run_r = r;
      run_g = g;
      run_b = b;
    } else // continue run
      run_len++;

    // new row -> write old to file
    if (color_count % img_w == 0) {
      std::uint16_t runs_in_row = rle_row.size();
      // write run count 
      ofile.write(reinterpret_cast<char*>(&runs_in_row), sizeof(runs_in_row));
      for (auto& run : rle_row) { // write runs
        ofile.write(reinterpret_cast<char*>(&run.len), sizeof(run.len));
        ofile.write(reinterpret_cast<char*>(&run.r), sizeof(run.r));
        ofile.write(reinterpret_cast<char*>(&run.g), sizeof(run.g));
        ofile.write(reinterpret_cast<char*>(&run.b), sizeof(run.b));
      }
      rle_row.clear();
    }
  }
}

void rle_decode(const char* filename) {

  std::ifstream file(filename, std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Fehler beim Lesen von " + std::string(filename));

  std::uint16_t w;
  file.read(reinterpret_cast<char*>(&w), sizeof(w));

  std::uint16_t h;
  file.read(reinterpret_cast<char*>(&h), sizeof(h));

  std::string ofilename = append_timestamp(filename);
  ofilename.append(".ppm");
  std::ofstream ofile(ofilename);
  if (!ofile.is_open())
    throw std::runtime_error("Fehler beim Schreiben von " + std::string(ofilename));
  //
  // write ppm header
  ofile <<
    "P3\n" <<
    "# comment test\n" <<
    w << "\n" <<
    h << "\n" <<
    255 << "\n";

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
    for (auto c=0; c<run_len; c++) {
      ofile << int(r) << " " << int(g) << " " << int(b) << "\n";
    }
    runs_per_line--;
  }
}

void print_usage() {
  std::cout << "Usage: huff-cli <option> <file>" << std::endl;
  std::cout << "options:" << std::endl;
  std::cout << "\t-1\thuffman encode file" << std::endl;
  std::cout << "\t-2\tdecode huffman encoded file" << std::endl;
  std::cout << "\t-3\trle encode ppm file" << std::endl;
  std::cout << "\t-4\trle decode ppm file" << std::endl;
  std::cout << "\t-5\thuffman+rle encode file" << std::endl;
  std::cout << "\t-6\tdecode huffman+rle encoded file" << std::endl;
}

int main(int argc, char* argv[]) {

  if (argc < 3) {
    print_usage();
    exit(1);
  }

  // get option
  std::string option {argv[1]};

  if (option == "-1") {
    huffman_encode(argv[2]);
    exit(0);
  }

  if (option == "-2") {
    huffman_decode(argv[2]);
    exit(0);
  }

  if (option == "-3") {
    rle_encode(argv[2]);
    exit(0);
  }

  if (option == "-4") {
    rle_decode(argv[2]);
    exit(0);
  }

  print_usage();

	return 0;
}
