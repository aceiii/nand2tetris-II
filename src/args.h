#ifndef NAND2TETRIS_ARGS_H
#define NAND2TETRIS_ARGS_H

#include <expected>
#include <string>


namespace app {
  struct Args {
    std::string filename;
    std::string log_level;
  };

  std::expected<Args, std::string> GetArgs(const std::string& name, const std::string& version, int argc, char** argv);
};


#endif //NAND2TETRIS_ARGS_H
