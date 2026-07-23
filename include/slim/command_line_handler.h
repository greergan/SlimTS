#pragma once
#include <string>
#include <vector>
namespace slim::command_line {
    std::vector<std::string> parse(int argc, char *argv[]);
}
