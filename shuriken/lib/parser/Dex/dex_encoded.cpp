//--------------------------------------------------------------------*- C++ -*-
// Shuriken-Analyzer: library for bytecode analysis.
// @author Farenain <kunai.static.analysis@gmail.com>
// @author Ernesto Java <javaernesto@gmail.com>
//
// @file encoded.cpp

#include "shuriken/parser/Dex/dex_encoded.h"

using namespace shuriken::parser::dex;

void EncodedArray::parse_encoded_array(common::ShurikenStream &stream,
                                       shuriken::parser::dex::DexTypes &types,
                                       shuriken::parser::dex::DexStrings &strings) {
    auto array_size = stream.read_uleb128();
    std::unique_ptr<EncodedValue> value = nullptr;

    for (std::uint64_t I = 0; I < array_size; ++I) {
        value = std::make_unique<EncodedValue>();
        value->parse_encoded_value(stream, types, strings);
        values.push_back(std::move(value));
    }
}

size_t EncodedArray::get_encodedarray_size() const {
    return values.size();
}

EncodedArray::it_encoded_value EncodedArray::get_encoded_values() {
    return make_range(values.begin(), values.end());
}

EncodedArray::it_const_encoded_value EncodedArray::get_encoded_values_const() {
    return make_range(values.begin(), values.end());
}

// AnnotationElement
AnnotationElement::AnnotationElement(std::string_view name,
                                std::unique_ptr < EncodedValue > value) :
        name(name), value(std::move(value)) {
}

std::string_view AnnotationElement::get_name() const {
    return name;
}

EncodedValue * AnnotationElement::get_value() {
    return value.get();
}


void EncodedAnnotation::parse_encoded_annotation(common::ShurikenStream & stream,
                                                 DexTypes & types,
                                                 DexStrings & strings) {
    std::unique_ptr<AnnotationElement> annotation;
    std::unique_ptr<EncodedValue> value;
    std::uint64_t name_idx;
    auto type_idx = stream.read_uleb128();
    auto size = stream.read_uleb128();

    type = types.get_type_by_id(static_cast<std::uint32_t>(type_idx));

    for (std::uint64_t I = 0; I < size; ++I) {
        // read first the name_idx
        name_idx = stream.read_uleb128();
        // then the EncodedValue
        value = std::make_unique<EncodedValue>();
        value->parse_encoded_value(stream, types, strings);
        // now create an annotation element
        annotation = std::make_unique<AnnotationElement>(strings.get_string_by_id(static_cast<uint32_t>(name_idx)), std::move(value));
        // add the anotation to the vector
        annotations.push_back(std::move(annotation));
    }

}

DVMType * EncodedAnnotation::get_annotation_type() {
    return type;
}

size_t EncodedAnnotation::get_number_of_annotations() const {
    return annotations.size();
}

EncodedAnnotation::it_annotation_elements EncodedAnnotation::get_annotations() {
    return make_range(annotations.begin(), annotations.end());
}

EncodedAnnotation::it_const_annotation_elements EncodedAnnotation::get_annotations_const() {
    return make_range(annotations.begin(), annotations.end());
}

AnnotationElement * EncodedAnnotation::get_annotation_by_pos(std::uint32_t pos) {
    if (pos >= annotations.size())
        throw std::runtime_error("Error pos annotation out of bound");
    return annotations[pos].get();
}

void EncodedValue::parse_encoded_value(common::ShurikenStream & stream,
                                       DexTypes & types,
                                       DexStrings & strings) {
    auto read_from_stream = [&](size_t size) {
        std::uint8_t aux;
        auto& value_data = std::get<std::vector<std::uint8_t>>(value);
        for(size_t I = 0; I <= size; ++I) {
            stream.read_data<std::uint8_t>(aux, sizeof(std::uint8_t));
            value_data.push_back(aux);
        }
    };

    // read the value format
    std::uint8_t aux;
    stream.read_data<std::uint8_t>(aux, sizeof(std::uint8_t));
    format = static_cast<shuriken::dex::TYPES::value_format>(aux & 0x1f);
    auto size = (aux >> 5);

    switch (format) {
        case shuriken::dex::TYPES::value_format::VALUE_BYTE:
        case shuriken::dex::TYPES::value_format::VALUE_SHORT:
        case shuriken::dex::TYPES::value_format::VALUE_CHAR:
        case shuriken::dex::TYPES::value_format::VALUE_INT:
        case shuriken::dex::TYPES::value_format::VALUE_FLOAT:
        case shuriken::dex::TYPES::value_format::VALUE_LONG:
        case shuriken::dex::TYPES::value_format::VALUE_DOUBLE:
        case shuriken::dex::TYPES::value_format::VALUE_STRING:
        case shuriken::dex::TYPES::value_format::VALUE_TYPE:
        case shuriken::dex::TYPES::value_format::VALUE_FIELD:
        case shuriken::dex::TYPES::value_format::VALUE_METHOD:
        case shuriken::dex::TYPES::value_format::VALUE_ENUM:
        {
            read_from_stream(size);
            break;
        }
        case shuriken::dex::TYPES::value_format::VALUE_ARRAY:
        {
            auto& array = std::get<std::unique_ptr < EncodedArray >>(value);
            array = std::make_unique<EncodedArray>();
            array->parse_encoded_array(stream, types, strings);
        }
        case shuriken::dex::TYPES::value_format::VALUE_ANNOTATION:
        {
            auto& annotation = std::get<std::unique_ptr < EncodedAnnotation >>(value);
            annotation = std::make_unique<EncodedAnnotation>();
            annotation->parse_encoded_annotation(stream, types, strings);
        }
        default:
            throw std::runtime_error("Value for format not implemented");
    }
}

shuriken::dex::TYPES::value_format EncodedValue::get_value_format() const {
    return format;
}

EncodedValue::it_data_buffer EncodedValue::get_data_buffer() {
    if (format == shuriken::dex::TYPES::value_format::VALUE_ARRAY ||
        format == shuriken::dex::TYPES::value_format::VALUE_ANNOTATION)
        throw std::runtime_error("Error value does not contain a data buffer");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return make_range(value_data.begin(), value_data.end());
}

EncodedArray * EncodedValue::get_array_data() {
    if (format == shuriken::dex::TYPES::value_format::VALUE_ARRAY)
        return std::get < std::unique_ptr < EncodedArray >> (value).get();
    return nullptr;
}

EncodedAnnotation * EncodedValue::get_annotation_data() {
    if (format == shuriken::dex::TYPES::value_format::VALUE_ANNOTATION)
        return std::get < std::unique_ptr < EncodedAnnotation >> (value).get();
    return nullptr;
}

std::int32_t EncodedValue::convert_data_to_int() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_INT)
        throw std::runtime_error("Error encoded value is not an int type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < std::int32_t * > (value_data.data()));
}

std::int64_t EncodedValue::convert_data_to_long() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_LONG)
        throw std::runtime_error("Error encoded value is not a long type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < std::int64_t * > (value_data.data()));
}

std::uint8_t EncodedValue::convert_data_to_byte() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_BYTE)
        throw std::runtime_error("Error encoded value is not a byte type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < std::uint8_t * > (value_data.data()));
}

std::int16_t EncodedValue::convert_data_to_short() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_SHORT)
        throw std::runtime_error("Error encoded value is not a short type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < std::int16_t * > (value_data.data()));
}

double EncodedValue::convert_data_to_double() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_DOUBLE)
        throw std::runtime_error("Error encoded value is not a double type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < double * > (value_data.data()));
}

float EncodedValue::convert_data_to_float() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_FLOAT)
        throw std::runtime_error("Error encoded value is not a float type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < float * > (value_data.data()));
}

std::uint16_t EncodedValue::convert_data_to_char() {
    if (format != shuriken::dex::TYPES::value_format::VALUE_CHAR)
        throw std::runtime_error("Error encoded value is not a char type");
    auto & value_data = std::get < std::vector < std::uint8_t >> (value);
    return * (reinterpret_cast < std::uint16_t * > (value_data.data()));
}

EncodedField::EncodedField(FieldID * field_idx, shuriken::dex::TYPES::access_flags flags)
        : field_idx(field_idx), flags(flags) {
  this->field_idx->set_encoded_field(this);
}

const FieldID* EncodedField::get_field() const {
    return field_idx;
}

FieldID* EncodedField::get_field() {
    return field_idx;
}

shuriken::dex::TYPES::access_flags EncodedField::get_flags() {
    return flags;
}

void EncodedField::set_initial_value(EncodedArray* initial_value) {
    this->initial_value = initial_value;
}

const EncodedArray* EncodedField::get_initial_value() const {
    return initial_value;
}

EncodedArray* EncodedField::get_initial_value() {
    return initial_value;
}

void EncodedCatchHandler::parse_encoded_catch_handler(common::ShurikenStream& stream,
                                                      DexTypes& types) {
    std::uint64_t type_idx, addr;

    /// the offset of the EncodedCatchHandler
    offset = static_cast<std::uint64_t>(stream.tellg());
    /// Size of the handlers
    size = stream.read_sleb128();

    for (size_t I = 0, S = std::abs(size); I < S; ++I) {
        type_idx = stream.read_uleb128();
        addr = stream.read_uleb128();

        handlers.push_back({
            .type = types.get_type_by_id(static_cast<uint32_t>(type_idx)),
            .idx = addr
        });
    }

    // A size of 0 means that there is a catch-all but no explicitly typed catches
    // And a size of -1 means that there is one typed catch along with a catch-all.
    if (size <= 0) {
        catch_all_addr = stream.read_uleb128();
    }
}

bool EncodedCatchHandler::has_explicit_typed_catches() const {
    if (size >= 0) return true; // user should check size of handlers
    return false;
}

std::int64_t EncodedCatchHandler::get_size() const {
    return size;
}

std::uint64_t EncodedCatchHandler::get_catch_all_addr() const {
    return catch_all_addr;
}

std::uint64_t EncodedCatchHandler::get_offset() const {
    return offset;
}

EncodedCatchHandler::it_handler_pairs EncodedCatchHandler::get_handle_pairs() {
    return make_range(handlers.begin(), handlers.end());
}

void CodeItemStruct::parse_code_item_struct(common::ShurikenStream& stream,
                                            DexTypes& types) {
    // instructions are read in chunks of 16 bits
    std::uint8_t instruction[2];
    size_t I;
    std::unique_ptr<EncodedCatchHandler> encoded_catch_handler;

    /// first read the code_item_struct_t
    stream.read_data<code_item_struct_t>(code_item, sizeof(code_item_struct_t));

    // now we can work with the values

    // first read the instructions for the CodeItem
    instructions_raw.reserve(code_item.insns_size*2);

    for (I = 0; I < code_item.insns_size; ++I) {
        // read the instruction
        stream.read_data<std::uint8_t[2]>(instruction, sizeof(std::uint8_t[2]));
        instructions_raw.push_back(instruction[0]);
        instructions_raw.push_back(instruction[1]);
    }

    if ((code_item.tries_size > 0) && // padding present in case tries_size > 0
        (code_item.insns_size % 2)) {   // and instructions size is odd

        // padding advance 2 bytes
        stream.seekg(sizeof(std::uint16_t), std::ios_base::cur);
    }

    // check if there are try-catch stuff
    if (code_item.tries_size > 0) {

        TryItem try_item = {0};

        for (I = 0; I < code_item.tries_size; ++I) {
            stream.read_data<TryItem>(try_item, sizeof(TryItem));
            try_items.push_back(try_item);
        }

        encoded_catch_handler_list_offset =
                static_cast<std::uint64_t>(stream.tellg());

        // now get the number of catch handlers
        encoded_catch_handler_size = stream.read_uleb128();

        for (I = 0; I < encoded_catch_handler_size; ++I) {
            encoded_catch_handler = std::make_unique<EncodedCatchHandler>();
            encoded_catch_handler->parse_encoded_catch_handler(stream, types);
            encoded_catch_handlers.push_back(std::move(encoded_catch_handler));
        }
    }
}

std::uint16_t CodeItemStruct::get_registers_size() const {
    return code_item.registers_size;
}

std::uint16_t CodeItemStruct::get_incomings_args() const {
    return code_item.ins_size;
}

std::uint16_t CodeItemStruct::get_outgoing_args() const {
    return code_item.outs_size;
}

std::uint16_t CodeItemStruct::get_number_try_items() const {
    return code_item.tries_size;
}

std::uint16_t CodeItemStruct::get_offset_to_debug_info() const {
    return code_item.debug_info_off;
}

std::uint16_t CodeItemStruct::get_instructions_size() const {
    return code_item.insns_size;
}

std::span<std::uint8_t> CodeItemStruct::get_bytecode() {
    std::span bytecode{instructions_raw};
    return bytecode;
}

CodeItemStruct::it_try_items CodeItemStruct::get_try_items() {
    return make_range(try_items.begin(), try_items.end());
}

std::uint64_t CodeItemStruct::get_encoded_catch_handler_offset() {
    return encoded_catch_handler_list_offset;
}

CodeItemStruct::it_encoded_catch_handlers CodeItemStruct::get_encoded_catch_handlers() {
    return make_range(encoded_catch_handlers.begin(), encoded_catch_handlers.end());
}

EncodedMethod::EncodedMethod(MethodID* method_id, shuriken::dex::TYPES::access_flags access_flags)
: method_id(method_id), access_flags(access_flags)
{}

void EncodedMethod::parse_encoded_method(common::ShurikenStream& stream,
                                         std::uint64_t code_off,
                                         DexTypes& types) {
    auto current_offset = stream.tellg();

    if (code_off > 0)
    {
        stream.seekg(code_off, std::ios_base::beg);
        // parse the code item
        code_item = std::make_unique<CodeItemStruct>();
        code_item->parse_code_item_struct(stream, types);
    }

    // return to current offset
    stream.seekg(current_offset, std::ios_base::beg);
}

const MethodID* EncodedMethod::getMethodID() const {
    return method_id;
}

MethodID* EncodedMethod::getMethodID() {
    return method_id;
}

shuriken::dex::TYPES::access_flags EncodedMethod::get_flags() {
    return access_flags;
}

const CodeItemStruct* EncodedMethod::get_code_item() const {
    return code_item.get();
}

CodeItemStruct* EncodedMethod::get_code_item() {
    return code_item.get();
}