#pragma once

#include "binaryninjaapi.h"
#include "shuriken_api/shuriken.h"
#include <cstdint>


namespace BinaryNinja {

class DEXViewType : public BinaryViewType {
  public:
    DEXViewType();
    virtual Ref<BinaryView> Create(BinaryView* data) override;
    virtual Ref<BinaryView> Parse(BinaryView* data) override;
    virtual bool IsTypeValidForData(BinaryView* data) override;
    virtual Ref<Settings> GetLoadSettingsForData(BinaryView* data) override;

  private:
    Ref<Logger> m_logger;
};
void InitDEXViewType();

class DEXView : public BinaryView {
  public:
    DEXView(BinaryView* data, bool parseOnly = false);
    virtual bool Init() override;

  protected:
    virtual uint64_t PerformGetEntryPoint() const override;
    virtual bool PerformIsExecutable() const override { return true; }
    virtual BNEndianness PerformGetDefaultEndianness() const override { return LittleEndian; }
    virtual bool PerformIsRelocatable() const override { return m_relocatable; }
    virtual size_t PerformGetAddressSize() const override;

  private:
    bool m_relocatable = false;
    bool m_parseOnly;
    bool m_backedByDatabase;
    uint64_t m_imageBase = 0;
    Ref<Logger> m_logger;
    Ref<Architecture> m_arch;
    Ref<Platform> m_platform;

    void buildStructures();
    void buildFunctions();
};

} // namespace BinaryNinja

class DataReader : public shurikenapi::IShurikenStream {
  public:
    DataReader(BinaryNinja::Ref<BinaryNinja::BinaryView> bv);
    std::size_t get_file_size() const override;
    void read_data(void* buffer, size_t read_size) override;
    void seek(size_t offset) override;
    void seek_safe(size_t offset) override;
    size_t tellg() const override;
    std::uint64_t read_uleb128() override;
    std::int64_t read_sleb128() override;
    std::string read_ansii_string(std::int64_t offset) override;
    std::string read_dex_string(std::int64_t offset) override;
  private:
    BinaryNinja::Ref<BinaryNinja::BinaryView> m_bv;
    std::unique_ptr<BinaryNinja::BinaryReader> m_reader;

};