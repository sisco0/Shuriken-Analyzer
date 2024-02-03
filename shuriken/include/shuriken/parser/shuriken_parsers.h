//--------------------------------------------------------------------*- C++ -*-
// Shuriken-Analyzer: library for bytecode analysis.
// @author Farenain <kunai.static.analysis@gmail.com>
//
// @file shuriken_parsers.h
// @brief DexHeader which contain the code to retrieve the parsers
// included in shuriken, this will return unique_ptr or shared_ptr
// objects for each parser.

#ifndef SHURIKENLIB_SHURIKEN_PARSERS_H
#define SHURIKENLIB_SHURIKEN_PARSERS_H

#include <memory>
#include "shuriken/common/shurikenstream.h"
#include "shuriken/parser/Dex/parser.h"

namespace shuriken {
    namespace parser {
        std::unique_ptr<dex::Parser> parse_dex(common::ShurikenStream &file);
        std::unique_ptr<dex::Parser> parse_dex(const std::string& file_path);
    }
}

#endif //SHURIKENLIB_SHURIKEN_PARSERS_H
