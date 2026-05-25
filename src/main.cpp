#include <iostream>
#include <string_view>

namespace {

constexpr int kSuccess = 0;
constexpr int kCliMisuse = 2;

void print_usage(std::ostream &output) {
  output << "Usage: sampan <command>\nCommands:\n"
         << "  version    Print the Sampan version\n";
}

} // namespace

int main(const int argc, char *argv[]) {
  if (argc == 2 && std::string_view{argv[1]} == "version") {
    
    std::cout << "Sampan 0.1.0 \n";
    return kSuccess;
  }
  print_usage(argc == 1 ? std::cout : std::cerr);
  return argc == 1 ? kSuccess : kCliMisuse;
}
