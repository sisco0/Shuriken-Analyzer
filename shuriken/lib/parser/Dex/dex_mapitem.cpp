//--------------------------------------------------------------------*- C++ -*-
// Shuriken-Analyzer: library for bytecode analysis.
// @author Farenain <kunai.static.analysis@gmail.com>
//
// @file mapitem.cpp

#include "shuriken/parser/Dex/dex_mapitem.h"
#include "shuriken/common/logger.h"


using namespace shuriken::parser::dex;



void DexMapList::parse_map_list(common::ShurikenStream& stream, std::uint32_t map_off) {
    auto my_logger = shuriken::logger();
    auto current_offset = stream.tellg();

    std::uint32_t size;

    map_item item;

    my_logger->info("Started parsing map_list at offset {}", map_off);

    stream.seekg(map_off, std::ios_base::beg);

    // first read the size
    stream.read_data<std::uint32_t>(size, sizeof(std::uint32_t));

    for (size_t I = 0; I < size; ++I)
    {
        stream.read_data<map_item>(item, sizeof(map_item));

        items[item.type] = {item.type, item.unused, item.size, item.offset};
    }

    my_logger->info("Finished parsing map_list");
    stream.seekg(current_offset, std::ios_base::beg);
}
