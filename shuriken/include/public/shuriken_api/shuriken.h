#ifndef SHURIKENLIB_API_H
#define SHURIKENLIB_API_H

#if defined(_WIN32) || defined(__WIN32__)
    #ifdef SHURIKENLIB_EXPORTS
        #define SHURIKENLIB_API __declspec(dllexport)
    #else
       #define SHURIKENLIB_API __declspec(dllimport)
    #endif
#else
    #define SHURIKENLIB_API
#endif

#include <memory>
#include <string>

namespace shurikenapi {

    class IShurikenStream {
    public:
        virtual ~IShurikenStream() = default;
        IShurikenStream &operator=(IShurikenStream &&) = delete;

        virtual std::size_t get_file_size() const = 0;
        virtual void read_data(void *buffer, size_t read_size) = 0;
        virtual void seek(size_t offset) = 0;
        virtual void seek_safe(size_t offset) = 0;
        virtual size_t tellg() const = 0;
        virtual std::uint64_t read_uleb128() = 0;
        virtual std::int64_t read_sleb128() = 0;
        virtual std::string read_ansii_string(std::int64_t offset) = 0;
        virtual std::string read_dex_string(std::int64_t offset) = 0;
    };

    SHURIKENLIB_API void parse_dex(shurikenapi::IShurikenStream *stream);
} // namespace shurikenapi



#endif // SHURIKENLIB_API_H
