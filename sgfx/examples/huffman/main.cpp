#include <string>

#include <fstream>
#include <iostream>

#include <cmath>
#include <bitset>
#include <algorithm>

//#define NDEBUG

void print_usage() {
  std::cerr << "Usage: huff-cli [options] file" << std::endl;
  std::cerr << "options:" << std::endl;
  std::cerr << "\t-d\tdecode" << std::endl;
}

void encode(const char* cfile) {

  std::ifstream file(cfile); 
  if (!file.is_open()) {
    std::cerr << "Error loading input file" << std::endl;
    exit(1);
  }

  // holds count of a byte == i
  int byte_type_count[256];
  std::fill_n(byte_type_count, 256, 0);

  auto char_to_byte = [](char c) { return c+128; };
  auto byte_to_char = [](unsigned char c) { return c-128; };
  char c; // [-128, 127]
  while (!file.eof()) {
    file.read(&c, sizeof(char));
    if (c == 10) continue;
    byte_type_count[char_to_byte(c)]++;
  }

#ifndef NDEBUG
  std::cout << "byte counts:" << std::endl;
  for (auto i=0; i<256; i++) {
    int c = byte_type_count[i];
    if (c > 0) {
      std::cout << c << "x" << i << " | ";
    }
  }
  std::cout << std::endl;
#endif

#ifndef NDEBUG
  std::cout << "sorting counts:" << std::endl;
  for (auto i=0; i<256; i++) {
    int c = byte_type_count[i];
    if (c > 0) { std::cout << c << " "; }
  }
  std::cout << std::endl;
#endif

  // sort byte_type_count
  // need this to map byte to count after sorting
  int byte_type[256];
  auto swap = [](auto i, auto j, int arr[256]) {
    int tmp = arr[i];
    arr[i] = arr[j];
    arr[j] = tmp;
  };
  for (auto i=0; i<256; i++)
    byte_type[i] = i;
  
  for (auto i=0; i<256; i++) {
    for (auto j=i+1; j<256; j++) {
      if (byte_type_count[j] < byte_type_count[i]) {
        swap(i, j, byte_type_count);
        swap(i, j, byte_type);
      }
    }
  }

#ifndef NDEBUG
  std::cout << "sorted counts:" << std::endl;
  for (auto i=0; i<256; i++) {
    int c = byte_type_count[i];
    if (c > 0) { std::cout << c << " "; }
  }
  std::cout << std::endl;
#endif

#ifndef NDEBUG
  std::cout << "sorted indices:" << std::endl;
  for (auto i=0; i<256; i++) {
    if (byte_type_count[i] > 0) { std::cout << byte_type[i] << " "; }
  }
  std::cout << std::endl;
#endif

  unsigned int byte_to_code_map[256];
  std::fill_n(byte_to_code_map, 256, 0);

  auto leftsum = [](auto start, int arr[256]) {
    std::cout << "leftsum(" << start << ")";
    auto sum = 0;
    for (auto i=start; i>=0; i--)
      sum += arr[i];
    std::cout << " = " << sum << std::endl;
    return sum;
  };
  auto pow = [](auto b, auto e) {
    if (e == 0) return 1;
    auto p = b;
    for (auto i=0; i<(e-1); i++)
      p *= b;
    return p;
  };

  int level[256];
  std::fill_n(level, 256, 0);
  int not_0_count = 0;
  for (auto i=1; i<256; i++) {
    if (byte_type_count[i] == 0) continue;
    if (++not_0_count < 2) continue;

    std::cout << "byte_type_count at " << i << " = " << byte_type_count[i] << std::endl;
    if (byte_type_count[i] <= leftsum(i-1, byte_type_count)) {
      for (auto j=i-1; j>=0; j--) {
        byte_to_code_map[byte_type[j]] |= pow(2, level[byte_type[j]]);
      }
    } else {
      byte_to_code_map[byte_type[i]] |= pow(2, level[byte_type[i]]);
    }
    for (auto j=i; j>=0; j--) {
      level[byte_type[j]]++;
    }
  }

#ifndef NDEBUG
  std::cout << "huff codes: " << std::endl;
  for (auto i=0; i<256; i++) {
    int b = byte_to_code_map[byte_type[i]];
    std::bitset<8> x(b);
    if (byte_type_count[i] > 0) {
      std::cout << char(byte_to_char(byte_type[i])) << ": " << x << std::endl;
    }
  }
  std::cout << std::endl;
#endif
}

void decode(const char* cfile) {
  std::cout << "Decoding " << cfile << std::endl;
}

int main(int argc, char* argv[]) {

  if (argc == 2) {
    encode(argv[1]);
    exit(0);
  }

  if (argc == 3) {
    std::string option {argv[1]};
    if (option == "-d")
    decode(argv[2]);
    exit(0);
  }

  print_usage();

	return 0;
}
