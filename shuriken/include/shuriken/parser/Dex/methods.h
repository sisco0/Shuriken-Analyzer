//--------------------------------------------------------------------*- C++ -*-
// Shuriken-Analyzer: library for bytecode analysis.
// @author Farenain <kunai.static.analysis@gmail.com>
//
// @file methods.h
// @brief Store information about the mehods in the DEX file

#ifndef SHURIKENLIB_METHODS_H
#define SHURIKENLIB_METHODS_H

#include "shuriken/parser/Dex/strings.h"
#include "shuriken/parser/Dex/types.h"
#include "shuriken/parser/Dex/protos.h"

#include <memory>
#include <vector>
#include <string_view>

namespace shuriken {
    namespace parser {
        namespace dex {
            class MethodID {
                /// @brief Class which method belongs to
                DVMType* class_;
                /// @brief Prototype of the current method
                ProtoID* protoId;
                /// @brief Name of the method
                std::string_view name;
                /// @brief Pretty name of the method with the prototype
                std::string pretty_name;
            public:
                /// @brief Constructor of the MethodID
                /// @param class_ class of the method
                /// @param return_type type returned by prototype of the method
                /// @param name_ name of the method
                MethodID(DVMType* class_, ProtoID* protoId, std::string_view name) :
                    class_(class_), protoId(protoId), name(name)
                {}

                /// @brief Destructor of MethodID, default constructor
                ~MethodID() = default;

                const DVMType* get_class() const {
                    return class_;
                }

                DVMType* get_class() {
                    return class_;
                }

                const ProtoID* get_prototype() const {
                    return protoId;
                }

                ProtoID* get_prototype() {
                    return protoId;
                }

                std::string_view get_method_name() {
                    return name;
                }

                std::string pretty_method();

            };

            class Methods {
            public:
                using method_ids_t = std::vector<std::unique_ptr<MethodID>>;
                using it_methods = iterator_range<method_ids_t::iterator>;
                using it_const_methods = iterator_range<const method_ids_t::iterator>;
            private:
                /// @brief List of methods from the DEX file
                method_ids_t method_ids;
            public:
                /// @brief Constructor of Methods, default Constructor
                Methods() = default;

                /// @bief Destructor of Methods, default Destructor
                ~Methods() = default;

                /// @brief Parse all the method ids objects.
                /// @param stream stream with the dex file
                /// @param types types objects
                /// @param strings strings objects
                /// @param methods_offset offset to the ids of the methods
                /// @param methods_size number of methods to read
                void parse_methods(
                        common::ShurikenStream& stream,
                        Types& types,
                        Protos& protos,
                        Strings& strings,
                        std::uint32_t methods_offset,
                        std::uint32_t methods_size
                );

                /// @brief Get an iterator for going through the methods from the DEX file
                /// @return iterator with the methods
                it_methods get_methods()  {
                    return make_range(method_ids.begin(), method_ids.end());
                }

                /// @brief Get a constant iterator for going through the methods from the DEX file
                /// @return constant iterator with the methods
                it_const_methods get_methods_const() {
                    return make_range(method_ids.begin(), method_ids.end());
                }

                /// @brief Get the number of methods from the DEX file
                /// @return number of methods
                size_t get_number_of_methods() const {
                    return method_ids.size();
                }

                /// @brief Get the MethodID pointer of the provided id.
                /// @param id id of the method to retrieve
                /// @return MethodID object
                MethodID* get_method_by_id(std::uint32_t id) {
                    if (id >= method_ids.size())
                        throw std::runtime_error("Error method id out of bound");
                    return method_ids.at(id).get();
                }

                /// @brief Dump the content of the methods as XML
                void to_xml(std::ofstream& fos);
            };
        }
    }
}

#endif //SHURIKENLIB_METHODS_H
