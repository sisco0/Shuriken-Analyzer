//--------------------------------------------------------------------*- C++ -*-
// Shuriken-Analyzer: library for bytecode analysis.
// @author Farenain <kunai.static.analysis@gmail.com>
//
// @file method_analysis.cpp

#include "shuriken/analysis/Dex/dex_analysis.h"
#include "shuriken/disassembler/Dex/internal_disassembler.h"
#include "shuriken/common/logger.h"

#include <iomanip>

using namespace shuriken::analysis::dex;

MethodAnalysis::MethodAnalysis(shuriken::parser::dex::EncodedMethod * encoded_method,
                               shuriken::disassembler::dex::DisassembledMethod * disassembled)
                               : method_encoded(encoded_method), disassembled(disassembled),
                               is_external(false)
                               {
    create_basic_blocks();
}

MethodAnalysis::MethodAnalysis(ExternalMethod * external_method)
    : method_encoded(external_method), is_external(true) {
}

void MethodAnalysis::dump_dot_file(std::string &file_path) {
    std::ofstream dot_file{file_path};

    dump_method_dot(dot_file);
}

bool MethodAnalysis::external() const {
    return is_external;
}

BasicBlocks & MethodAnalysis::get_basic_blocks() {
    return basic_blocks;
}

shuriken::disassembler::dex::DisassembledMethod * MethodAnalysis::get_disassembled_method() {
    return disassembled;
}

bool MethodAnalysis::is_android_api() const {
    if (!is_external) return false;

    auto class_name_view = this->get_class_name();

    auto it = std::find_if(known_apis.begin(), known_apis.end(), [&](std::string_view api) -> bool {
        return api.compare(class_name_view);
    });

    if (it == known_apis.end()) return false;
    return true;
}

std::string_view MethodAnalysis::get_name() const {
    return is_external ?
           std::get<ExternalMethod *>(method_encoded)->get_name_idx()
           :  std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded)->getMethodID()->get_method_name();
}

std::string_view MethodAnalysis::get_descriptor() const {
    return is_external ?
        std::get<ExternalMethod *>(method_encoded)->get_proto_idx()
        : std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded)->getMethodID()->get_prototype()->get_dalvik_prototype();
}

shuriken::dex::TYPES::access_flags MethodAnalysis::get_access_flags() const {
    return access_flags;
}

std::string_view MethodAnalysis::get_class_name() const {
    return is_external ?
           std::get<ExternalMethod *>(method_encoded)->get_class_idx()
           : std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded)->getMethodID()->get_class()->get_raw_type();
}

std::string_view MethodAnalysis::get_full_name() const {
    return is_external ?
        std::get<ExternalMethod*>(method_encoded)->pretty_method_name()
        : std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded)->getMethodID()->dalvik_name_format();
}

shuriken::disassembler::dex::Instruction * MethodAnalysis::get_instruction_by_addr(std::uint64_t addr) {
    for (auto & instr : disassembled->get_instructions()) {
        if (instr->get_address() == addr) return instr.get();
    }
    return nullptr;
}

shuriken::disassembler::dex::it_instructions MethodAnalysis::instructions() {
    if (disassembled)
        return disassembled->get_instructions();
    throw std::runtime_error("Error getting instructions, not disassembled method");
}

shuriken::parser::dex::EncodedMethod * MethodAnalysis::get_encoded_method() {
    return std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded);
}

ExternalMethod * MethodAnalysis::get_external_method() {
    return std::get<ExternalMethod*>(method_encoded);
}

shuriken::iterator_range<class_field_idx_iterator_t>  MethodAnalysis::get_xrefread() {
    return make_range(xrefread.begin(), xrefread.end());
}

shuriken::iterator_range<class_field_idx_iterator_t> MethodAnalysis::get_xrefwrite() {
    return make_range(xrefwrite.begin(), xrefwrite.end());
}

shuriken::iterator_range<class_method_idx_iterator_t> MethodAnalysis::get_xrefto() {
    return make_range(xrefto.begin(), xrefto.end());
}

shuriken::iterator_range<class_method_idx_iterator_t> MethodAnalysis::get_xreffrom() {
    return make_range(xreffrom.begin(), xreffrom.end());
}

shuriken::iterator_range<class_idx_iterator_t> MethodAnalysis::get_xrefnewinstance() {
    return make_range(xrefnewinstance.begin(), xrefnewinstance.end());
}

shuriken::iterator_range<class_idx_iterator_t> MethodAnalysis::get_xrefconstclass() {
    return make_range(xrefconstclass.begin(), xrefconstclass.end());
}

void MethodAnalysis::add_xrefread(ClassAnalysis *c, FieldAnalysis *f, std::uint64_t offset) {
    xrefread.emplace_back(c, f, offset);
}

void MethodAnalysis::add_xrefwrite(ClassAnalysis *c, FieldAnalysis *f, std::uint64_t offset) {
    xrefwrite.emplace_back(c, f, offset);
}

void MethodAnalysis::add_xrefto(ClassAnalysis *c, MethodAnalysis *m, std::uint64_t offset) {
    xrefto.emplace_back(c, m, offset);
}

void MethodAnalysis::add_xreffrom(ClassAnalysis *c, MethodAnalysis *m, std::uint64_t offset) {
    xreffrom.emplace_back(c, m, offset);
}

void MethodAnalysis::add_xrefnewinstance(ClassAnalysis *c, std::uint64_t offset) {
    xrefnewinstance.emplace_back(c, offset);
}

void MethodAnalysis::add_xrefconstclass(ClassAnalysis *c, std::uint64_t offset) {
    xrefconstclass.emplace_back(c, offset);
}

void MethodAnalysis::create_basic_blocks() {
    auto log = logger();

    /// utilities to create the basic blocks
    std::vector<std::int64_t> entry_points;
    std::unordered_map<std::uint64_t, std::vector<std::int64_t>> target_jumps;
    shuriken::disassembler::dex::Disassembler internal_disassembler;

    // useful variables
    auto method = std::get<shuriken::parser::dex::EncodedMethod *>(method_encoded);

    log->debug("Started creating basic blocks from method {}", get_full_name());

    DVMBasicBlock * current = nullptr;

    for (const auto & instruction : disassembled->get_instructions()) {
        auto operation = shuriken::disassembler::dex::InstructionUtils::get_operation_type_from_opcode(
                static_cast<shuriken::disassembler::dex::DexOpcodes::opcodes>(instruction->get_instruction_opcode())
                );

        if (operation == shuriken::disassembler::dex::DexOpcodes::CONDITIONAL_BRANCH_DVM_OPCODE
            || operation == shuriken::disassembler::dex::DexOpcodes::UNCONDITIONAL_BRANCH_DVM_OPCODE
            || operation == shuriken::disassembler::dex::DexOpcodes::MULTI_BRANCH_DVM_OPCODE) {
            auto idx = instruction->get_address();
            auto ins = instruction.get();

            auto v = internal_disassembler.determine_next(ins, idx);
            target_jumps[idx] = std::move(v);
            entry_points.insert(entry_points.end(), target_jumps[idx].begin(), target_jumps[idx].end());
        }
    }

    // now analyze the exceptions and obtain the entry point addresses
    for (const auto & except : disassembled->get_exceptions()) {
        /// entry point of try values can start in the middle
        /// of a block
        for (const auto &handler : except.handler) {
            entry_points.push_back(handler.handler_start_addr);
        }
    }

    std::int64_t start = 0, end = 0;

    for (const auto & instruction : disassembled->get_instructions()) {
        auto idx = instruction->get_address();
        auto ins = instruction.get();


        if (std::find(entry_points.begin(), entry_points.end(), idx) != entry_points.end() &&
            start != end) {
            auto prev = current;
            current = new DVMBasicBlock(disassembled->get_ref_to_instructions(start, end));

            if (prev != nullptr) {
                /// if last instruction is not a terminator
                /// we must create an edge because it comes
                /// from a fallthrough block
                if (!prev->get_terminator())
                    basic_blocks.add_edge(prev, current);
                /// in other case, just add the node, and later
                /// will be added the edge
                else
                    basic_blocks.add_node(current);
            }
        }

        /// update the end pointer
        end = ins->get_address();
    }
    
    auto & last_instr = disassembled->get_instructions_container().back();
    auto out_range = last_instr->get_address() + last_instr->get_instruction_length();

    /// add the jump targets
    for (const auto & jump_target : target_jumps) {
        auto src_idx = jump_target.first;
        auto src = basic_blocks.get_basic_block_by_idx(src_idx);
        /// need to check how target jump is generated
        if (src_idx >= out_range || src == nullptr)
            continue;

        for (auto dst_idx : jump_target.second) {
            auto dst = basic_blocks.get_basic_block_by_idx(dst_idx);
            /// need to check how target jump is generated
            if (dst_idx >= out_range || dst == nullptr)
                continue;
            basic_blocks.add_edge(src, dst);
        }
    }

    /// add the exceptions
    for (const auto & except : disassembled->get_exceptions()) {
        auto try_bb = basic_blocks.get_basic_block_by_idx(except.try_value_start_addr);
        try_bb->set_try_block(true);

        for (const auto & handler : except.handler) {
            auto catch_bb = basic_blocks.get_basic_block_by_idx(handler.handler_start_addr);
            catch_bb->set_catch_block(true);
            catch_bb->set_handler_type(handler.handler_type);
        }
    }
}

void MethodAnalysis::dump_instruction_dot(std::ofstream &dot_file, disassembler::dex::Instruction *instr) {
    dot_file << "<tr><td align=\"left\">";

    dot_file << std::right << std::setfill('0') << std::setw(8) << std::hex << instr->get_address() << "  ";

    const auto &opcodes = instr->get_opcodes();

    if (opcodes.size() > 8) {
        auto remaining = 8 - (opcodes.size() % 8);

        size_t aux = 0;

        for (const auto opcode : opcodes)
        {
            dot_file << std::right << std::setfill('0') << std::setw(2) << std::hex << (std::uint32_t)opcode << " ";
            aux++;
            if (aux % 8 == 0)
            {
                dot_file << "\\l"
                         << "          ";
            }
        }

        for (std::uint8_t i = 0; i < remaining; i++)
            dot_file << "   ";
    } else {
        for (const auto opcode : opcodes)
            dot_file << std::right << std::setfill('0') << std::setw(2) << std::hex << (std::uint32_t)opcode << " ";

        for (std::uint8_t i = 0, remaining_size = 8 - opcodes.size(); i < remaining_size; ++i)
            dot_file << "   ";
    }

    auto content = std::string(instr->print_instruction());

    while (content.find('\"') != std::string::npos)
        content.replace(content.find('\"'), 1, "'", 1);

    while (content.find('<') != std::string::npos)
        content.replace(content.find('<'), 1, "&lt;", 4);

    while (content.find('>') != std::string::npos)
        content.replace(content.find('>'), 1, "&gt;", 4);

    dot_file << content << "</td></tr>\n";
}

void MethodAnalysis::dump_block_dot(std::ofstream &dot_file, DVMBasicBlock *bb) {
    dot_file << "\"" << bb->get_name() << "\""
             << "[label=<<table border=\"0\" cellborder=\"0\" cellspacing=\"0\">\n";

    dot_file << R"(<tr><td colspan="2" align="left"><b>[)" << bb->get_name() << "]</b></td></tr>\n";
    if (bb->is_try_block())
        dot_file << "<tr><td colspan=\"2\" align=\"left\"><b>try_:</b></td></tr>\n";
    else if (bb->is_catch_block())
        dot_file << "<tr><td colspan=\"2\" align=\"left\"><b>catch_:</b></td></tr>\n";

    for (auto instr : bb->get_instructions())
        dump_instruction_dot(dot_file, instr);

    dot_file << "</table>>];\n\n";
}

void MethodAnalysis::dump_method_dot(std::ofstream &dot_file) {
    shuriken::disassembler::dex::Disassembler d;

    auto full_name = get_full_name();

    // first dump the headers of the dot file
    dot_file << "digraph \"" << full_name << "\"{\n";
    dot_file << "style=\"dashed\";\n";
    dot_file << "color=\"black\";\n";
    dot_file << "label=\"" << full_name << "\";\n";
    dot_file << "node [shape=box, style=filled, fillcolor=lightgrey, fontname=\"Courier\", fontsize=\"10\"];\n";
    dot_file << "edge [color=black, arrowhead=open];\n";

    for (auto bb : basic_blocks.nodes())
        dump_block_dot(dot_file, bb);

    for (const auto &edge : basic_blocks.edges())
    {
        auto terminator_instr = edge.first->get_terminator();

        auto operation = shuriken::disassembler::dex::InstructionUtils::get_operation_type_from_opcode(
                static_cast<shuriken::disassembler::dex::DexOpcodes::opcodes>(terminator_instr->get_instruction_opcode())
        );

        if (terminator_instr &&
            operation == shuriken::disassembler::dex::DexOpcodes::CONDITIONAL_BRANCH_DVM_OPCODE)
        {
            auto second_block_addr = edge.second->get_first_address();

            if (second_block_addr ==
                (terminator_instr->get_address() + terminator_instr->get_instruction_length())) // falthrough branch
                dot_file << "\"" << edge.first->get_name() << "\" -> "
                         << "\"" << edge.second->get_name() << "\" [style=\"solid,bold\",color=red,weight=10,constraint=true];\n";
            else
                dot_file << "\"" << edge.first->get_name() << "\" -> "
                         << "\"" << edge.second->get_name() << "\" [style=\"solid,bold\",color=green,weight=10,constraint=true];\n";
        }
        else if (terminator_instr &&
                operation == shuriken::disassembler::dex::DexOpcodes::UNCONDITIONAL_BRANCH_DVM_OPCODE)
        {
            dot_file << "\"" << edge.first->get_name() << "\" -> "
                     << "\"" << edge.second->get_name() << "\" [style=\"solid,bold\",color=blue,weight=10,constraint=true];\n";
        }
        else
            dot_file << "\"" << edge.first->get_name() << "\" -> "
                     << "\"" << edge.second->get_name() << "\" [style=\"solid,bold\",color=black,weight=10,constraint=true];\n";
    }

    dot_file << "}";
}