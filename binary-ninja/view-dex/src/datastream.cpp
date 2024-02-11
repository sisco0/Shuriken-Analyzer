#include "plugin.h"

DataReader::DataReader(BinaryNinja::Ref<BinaryNinja::BinaryView> bv) : m_bv(bv) {

    m_reader = std::make_unique<BinaryNinja::BinaryReader>(m_bv, LittleEndian);
}

std::size_t DataReader::get_file_size() const { return m_bv->GetLength(); }

void DataReader::read_data(void* buffer, size_t read_size) { m_reader->Read(buffer, read_size); }

void DataReader::seek(size_t offset) { m_reader->Seek(offset); }

void DataReader::seek_safe(size_t offset) {
    if (offset >= get_file_size())
        throw std::runtime_error("offset provided is out of bound");

    seek(offset);
}

size_t DataReader::tellg() const { return m_reader->GetOffset(); }

std::uint64_t DataReader::read_uleb128() {
    std::uint64_t value = 0;
    unsigned shift = 0;
    std::int8_t byte_read;

    do {
        read_data(&byte_read, sizeof(std::int8_t));
        value |= static_cast<std::uint64_t>(byte_read & 0x7f) << shift;
        shift += 7;
    } while (byte_read & 0x80);

    return value;
}

std::int64_t DataReader::read_sleb128() {
    std::int64_t value = 0;
    unsigned shift = 0;
    std::int8_t byte_read;

    do {
        read_data(&byte_read, sizeof(std::int8_t));
        value |= static_cast<std::uint64_t>(byte_read & 0x7f) << shift;
        shift += 7;
    } while (byte_read & 0x80);

    // sign extend negative numbers
    if ((byte_read & 0x40))
        value |= static_cast<std::int64_t>(-1) << shift;

    return value;
}

std::string DataReader::read_ansii_string(std::int64_t offset) {
    std::string new_str = "";
    std::int8_t character = -1;
    std::uint64_t utf16_size;

    auto int8_s = sizeof(std::int8_t);
    // save current offset
    auto curr_offset = tellg();

    // set the offset to the given offset
    seek(offset);

    utf16_size = read_uleb128();

    while (utf16_size-- > 0) {
        read_data(reinterpret_cast<char*>(&character), int8_s);
        new_str += static_cast<char>(character);
    }

    // return again
    seek(curr_offset);
    return new_str;
}

std::string DataReader::read_dex_string(std::int64_t offset) {
    std::string new_str = "";
    std::int8_t character = -1;
    std::uint64_t utf16_size;
    auto int8_s = sizeof(std::int8_t);
    // save current offset
    auto current_offset = tellg();

    // set the offset to the given offset
    seek(offset);

    utf16_size = read_uleb128();

    while (utf16_size--) {
        read_data(reinterpret_cast<char*>(&character), int8_s);
        new_str += static_cast<char>(character);
    }

    // return to offset
    seek(current_offset);
    // return the new string
    return new_str;
}
