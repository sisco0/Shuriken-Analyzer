//
// Created by fare9 on 30/12/23.
//

#include <shuriken/shuriken_cpp_core.h>
#include <fmt/core.h>
#include <iostream>
#include <vector>

void show_help(std::string& prog_name) {
    fmt::println("USAGE: {} <file_to_analyze> [-h] [-c] [-f] [-m] [-b]", prog_name);
    fmt::println("\t-h: show file header");
    fmt::println("\t-c: show classes from file");
    fmt::println("\t-f: show fields from classes (it needs -c)");
    fmt::println("\t-m: show methods from classes (it needs -c)");
    fmt::println("\t-b: show bytecode from methods (it needs -m)");
    fmt::println("\t-D: show the disassembled code from methods (it needs -m)");
}

void print_header(const shurikenapi::DexHeader& header);
void print_classes(const shurikenapi::IClassManager& classManager);
void print_field(const shurikenapi::ClassField& f, size_t index);
void print_method(const shurikenapi::ClassMethod& method, size_t index);
void print_code(std::span<std::uint8_t> bytecode);

bool headers = false;
bool show_classes = false;
bool methods = false;
bool fields = false;
bool code = false;
bool disassembly = false;

//std::unique_ptr<shuriken::disassembler::dex::DexDisassembler> disassembler;

int
main(int argc, char ** argv) {
    std::vector<std::string> args {argv, argv + argc};

    if (args.size() == 1) {
        show_help(args[0]);
        return -1;
    }

    for (auto & s : args) {
        if (s == "-h")
            headers = true;
        else if (s == "-c")
            show_classes = true;
        else if (s == "-m")
            methods = true;
        else if (s == "-f")
            fields = true;
        else if (s == "-b")
            code = true;
        else if (s == "-D")
            disassembly = true;
    }

    try {
        std::unique_ptr<shurikenapi::IDex> parsed_dex = shurikenapi::parse_dex(args[1]);
        if (!parsed_dex) {
            fmt::println("Failed to parse dex");
            return -1;
        }

        if (headers) print_header(parsed_dex->getHeader());
        if (show_classes) print_classes(parsed_dex->getClassManager());
        /*
        if (disassembly) {
            disassembler = std::make_unique<shuriken::disassembler::dex::DexDisassembler>(parsed_dex.get());
            disassembler->disassembly_dex();
        }
        */
    } catch (std::runtime_error& re) {
        fmt::println("Exception: {}", re.what());
    }

}

void print_header(const shurikenapi::DexHeader& dex_header) {
    fmt::println("Dex Header:");
    fmt::print("\tMagic:");
    for (auto b : dex_header.magic) {
        if (isprint(b))
            fmt::print(" {:02X} ({:c})", b, b);
        else
            fmt::print(" {:02X} ( )", b, b);
    }
    fmt::println("\n\tChecksum:              0x{:X}", static_cast<std::uint32_t>(dex_header.checksum));
    fmt::print("\tSignature:");
    for (auto b : dex_header.signature) {
        fmt::print(" {:02X}", b);
    }
    fmt::print("\n\tFile Size:             {}\n", dex_header.file_size);
    fmt::print("\tHeader Size:           {}\n", dex_header.header_size);
    fmt::print("\tEndian Tag:            0x{:X}\n", dex_header.endian_tag);
    fmt::print("\tLink offset:           0x{:X}\n", dex_header.link_off);
    fmt::print("\tLink Size:             {}\n", dex_header.link_size);
    fmt::print("\tMap offset:            0x{:X}\n", dex_header.map_off);
    fmt::print("\tString ids offset:     0x{:X}\n", dex_header.string_ids_off);
    fmt::print("\tString ids size:       {}\n", dex_header.string_ids_size);
    fmt::print("\tType ids offset:       0x{:X}\n", dex_header.type_ids_off);
    fmt::print("\tType ids size:         {}\n", dex_header.type_ids_size);
    fmt::print("\tProto ids offset:      0x{:X}\n", dex_header.proto_ids_off);
    fmt::print("\tProto ids size:        {}\n", dex_header.proto_ids_size);
    fmt::print("\tField ids offset:      0x{:X}\n", dex_header.field_ids_off);
    fmt::print("\tField ids size:        {}\n", dex_header.field_ids_size);
    fmt::print("\tMethod ids offset:     0x{:X}\n", dex_header.method_ids_off);
    fmt::print("\tMethod ids size:       {}\n", dex_header.method_ids_size);
    fmt::print("\tClass ids offset:      0x{:X}\n", dex_header.class_defs_off);
    fmt::print("\tClass ids size:        {}\n", dex_header.class_defs_size);
    fmt::print("\tData ids offset:       0x{:X}\n", dex_header.data_off);
    fmt::print("\tData ids size:         {}\n", dex_header.data_size);
}



void print_classes(const shurikenapi::IClassManager& classManager) {
    auto classes = classManager.getAllClasses();
    size_t I = 0;
    for (auto& c : classes) {
        fmt::print("Class #{} data:\n", I);

        fmt::print("\tClass name:            {}\n", c.definitions.class_name);
        fmt::print("\tSuper class:           {}\n", c.definitions.super_class);
        fmt::print("\tSource file:           {}\n", c.definitions.source_file);
        fmt::print("\tAccess flags:          0x{:X} ({})\n", static_cast<std::uint32_t>(c.definitions.flags), 
                        shurikenapi::utils::get_types_as_string(c.definitions.flags));

        fmt::print("\tStatic Fields:\n");
        int staticIdx = 0;
        for ( auto& f : c.fields) {
            if (f.isStatic)
                print_field(f, staticIdx++);
        }
        fmt::print("\tInstance Fields:\n");
        int instanceIdx = 0;
        for ( auto& f : c.fields) {
            if (f.isInstance)
                print_field(f, instanceIdx++);
        }

        fmt::print("\tDirect Methods:\n");
        int directIdx = 0;
        for (auto& m : c.methods) {
            if (m.isDirect)
                print_method(m, directIdx++);
        }
        fmt::print("\tVirtual Methods:\n");
        int virtualIdx = 0;
        for (auto& m : c.methods) {
            if (m.isVirtual)
                print_method(m, virtualIdx++);
        }

    }
}

void print_field(const shurikenapi::ClassField& f, size_t index) {
    fmt::print("\t\tField #{}\n", index);
    fmt::print("\t\t\tName:            {}\n", f.name);
    fmt::print("\t\t\tType:            {}\n", f.type_value);
    fmt::print("\t\t\tAccess Flags:    {} ({})\n", static_cast<std::uint32_t>(f.flags),
            shurikenapi::utils::get_types_as_string(f.flags));

}

void print_method(const shurikenapi::ClassMethod& method, size_t index) {
    fmt::print("\t\tMethod #{}\n", index);

    fmt::print("\t\t\tMethod name:    {}\n", method.name);
    fmt::print("\t\t\tPrototype:      {}\n", method.prototype);

    fmt::print("\t\t\tAccess Flags:   0x{:X} ({})\n", static_cast<std::uint32_t>(method.flags),
               shurikenapi::utils::get_types_as_string(method.flags));

    fmt::print("\t\t\tCode size:      {}\n", method.code_size);
    print_code(method.code);
}

void print_code(std::span<std::uint8_t> bytecode) {
    fmt::print("\t\t\tCode: ");
    size_t j = 0;
    for (auto b : bytecode) {
        fmt::print("{:02X} ", b);
        if (j++ == 8) {
            j = 0;
            fmt::print("\n\t\t\t      ");
        }
    }
    fmt::print("\n");
}
