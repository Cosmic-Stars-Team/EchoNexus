#ifndef ECHONEXUS_UTILS_PARSE_HPP
#define ECHONEXUS_UTILS_PARSE_HPP

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

#include <types/common.hpp>

namespace echo::utils {
    [[nodiscard]] inline type::map_t parse_query(
        std::string_view query
    ) {
        type::map_t out;

        std::size_t begin = 0;
        while (begin <= query.size()) {
            const std::size_t amp       = query.find('&', begin);
            const std::size_t len       = (amp == std::string_view::npos) ? (query.size() - begin) : (amp - begin);
            const std::string_view part = query.substr(begin, len);

            if (!part.empty()) {
                const std::size_t eq = part.find('=');

                std::string key;
                std::string value;

                if (eq == std::string_view::npos) {
                    key = std::string(part);
                } else {
                    key   = std::string(part.substr(0, eq));
                    value = std::string(part.substr(eq + 1));
                }

                if (!key.empty()) out[std::move(key)] = std::move(value);
            }

            if (amp == std::string_view::npos) break;

            begin = amp + 1;
        }

        return out;
    }

    [[nodiscard]] inline std::pair<std::string, type::map_t> parse_target(
        std::string_view target
    ) {
        const std::size_t query_pos = target.find('?');
        if (query_pos == std::string_view::npos) {
            if (target.empty()) return {"/", {}};
            return {std::string(target), {}};
        }

        std::string path = std::string(target.substr(0, query_pos));
        if (path.empty()) path = "/";

        const std::string_view query = target.substr(query_pos + 1);
        return {std::move(path), parse_query(query)};
    }
} // namespace echo::utils

#endif // ECHONEXUS_UTILS_PARSE_HPP
