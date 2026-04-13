#ifndef ECHONEXUS_TYPES_REQUEST_HPP
#define ECHONEXUS_TYPES_REQUEST_HPP

#include <algorithm>
#include <any>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include <types/common.hpp>

namespace echo::type {
    struct request {
        /// @brief Request method, for example GET/POST/PUT.
        std::string method;
        /// @brief Raw request target, for example /users?id=1.
        std::string target;
        /// @brief Parsed path part, for example /users.
        std::string path;
        /// @brief Parsed query parameters.
        map_t query;
        /// @brief Request headers.
        map_t headers;
        /// @brief Raw request body.
        std::string body;
        /// @brief HTTP version in numeric form, for example 11 for HTTP/1.1.
        std::uint16_t http_version = 11;
        /// @brief Remote endpoint address.
        std::string remote_addr;
        /// @brief Remote endpoint port.
        std::uint16_t remote_port = 0;
        /// @brief Middleware shared context.
        context_t context;

        [[nodiscard]] const std::string* get_query(
            std::string_view key
        ) const {
            const auto it = query.find(std::string(key));
            if (it == query.end()) return nullptr;

            return &it->second;
        }

        [[nodiscard]] const std::string* get_header(
            std::string_view key
        ) const {
            std::string key_str(key);
            const auto exact_it = headers.find(key_str);
            if (exact_it != headers.end()) return &exact_it->second;

            std::string lowered_key(key_str);
            std::transform(lowered_key.begin(), lowered_key.end(), lowered_key.begin(), [](const unsigned char ch) {
                return static_cast<char>(std::tolower(ch));
            });

            for (const auto& [header_key, header_value] : headers) {
                if (header_key.size() != lowered_key.size()) continue;

                bool equal = true;
                for (std::size_t index = 0; index < header_key.size(); ++index) {
                    if (static_cast<char>(std::tolower(static_cast<unsigned char>(header_key[index]))) != lowered_key[index]) {
                        equal = false;
                        break;
                    }
                }

                if (equal) return &header_value;
            }

            return nullptr;
        }

        template <typename T>
        void set_ctx(
            std::string key,
            T&& value
        ) {
            context[std::move(key)] = std::any(std::forward<T>(value));
        }

        template <typename T>
        [[nodiscard]] T* get_ctx(
            std::string_view key
        ) {
            const auto it = context.find(std::string(key));
            if (it == context.end()) return nullptr;

            return std::any_cast<T>(&it->second);
        }

        template <typename T>
        [[nodiscard]] const T* get_ctx(
            std::string_view key
        ) const {
            const auto it = context.find(std::string(key));
            if (it == context.end()) return nullptr;

            return std::any_cast<T>(&it->second);
        }
    };
} // namespace echo::type

#endif // ECHONEXUS_TYPES_REQUEST_HPP
