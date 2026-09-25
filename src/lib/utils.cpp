#include "utils.hpp"

#include <sstream>
#include <fstream>

std::string read_text_file(const std::string &shader_path) {
  std::ifstream infile(shader_path);
  if (infile.is_open()) {
    std::stringstream buffer;
    buffer << infile.rdbuf();
    const std::string output = buffer.str();
    infile.close();
    return std::move(output);
  }
  return "";
}