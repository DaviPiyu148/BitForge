#include "bitforge/cli/runner.hpp"
#include <iostream>

int main(int argc, char* argv[]) {
    const auto exit_code = bitforge::cli::run_cli(argc, argv, std::cout, std::cerr);
    return static_cast<int>(exit_code);
}
