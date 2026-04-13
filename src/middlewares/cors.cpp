#include <algorithm>
#include <cctype>
#include <middlewares/cors.hpp>
#include <stdexcept>
#include <utility>

auto echo::middlewares::allow_method::operator()() const -> std::string {
    switch (method_) {
    case GET:
        return "GET";
    case POS:
        return "POST";
    case PUT:
        return "PUT";
    case PAT:
        return "PATCH";
    case DEL:
        return "DELETE";
    case OPT:
        return "OPTIONS";
    case HEA:
        return "HEAD";
    case TRC:
        return "TRACE";
    }

    throw std::invalid_argument("invalid allow_method value");
}

echo::middlewares::cors::cors() = default;

echo::middlewares::cors::cors(
    preset policy
) {
    switch (policy) {
    case preset::strict:
        break;
    case preset::permissive:
        any_origin_ = true;
        any_method_ = true;
        any_header_ = true;
        any_exposed_header_ = true;
        break;
    case preset::development:
        mirror_origin_ = true;
        mirror_method_ = true;
        mirror_request_headers_ = true;
        allow_credentials_ = true;
        break;
    }
}

auto echo::middlewares::cors::allow_origin(
    std::string origin
) -> cors& {
    if (origin == "*") {
        if (allow_credentials_) {
            throw std::invalid_argument("cannot allow wildcard origin when credentials are enabled");
        }

        any_origin_ = true;
        allowed_origins_.clear();
        mirror_origin_ = false;

        return *this;
    }

    if (any_origin_) {
        any_origin_ = false;
    }

    if (mirror_origin_) {
        mirror_origin_ = false;
    }

    if (std::find(allowed_origins_.begin(), allowed_origins_.end(), origin) == allowed_origins_.end()) {
        allowed_origins_.push_back(std::move(origin));
    }

    return *this;
}

auto echo::middlewares::cors::allow_origins(
    std::vector<std::string> origins
) -> cors& {
    if (std::find(origins.begin(), origins.end(), "*") != origins.end()) {
        allow_origin("*");
        return *this;
    }

    for (auto& origin : origins) {
        allow_origin(origin);
    }

    return *this;
}

auto echo::middlewares::cors::allow_method(
    echo::middlewares::allow_method method
) -> cors& {
    if (any_method_) {
        any_method_ = false;
    }

    if (mirror_method_) {
        mirror_method_ = false;
    }

    const auto method_name = method();
    bool found = false;
    for (const auto& existing : allowed_methods_) {
        if (existing() == method_name) {
            found = true;
            break;
        }
    }

    if (!found) {
        allowed_methods_.push_back(std::move(method));
    }

    return *this;
}

auto echo::middlewares::cors::allow_methods(
    std::vector<echo::middlewares::allow_method> methods
) -> cors& {
    for (auto& method : methods) {
        allow_method(method);
    }

    return *this;
}

auto echo::middlewares::cors::allow_header(
    std::string header
) -> cors& {
    if (header == "*") {
        if (allow_credentials_) {
            throw std::invalid_argument("cannot allow wildcard headers when credentials are enabled");
        }

        any_header_ = true;
        allowed_headers_.clear();
        mirror_request_headers_ = false;

        return *this;
    }

    if (any_header_) {
        any_header_ = false;
    }

    if (mirror_request_headers_) {
        mirror_request_headers_ = false;
    }

    if (std::find(allowed_headers_.begin(), allowed_headers_.end(), header) == allowed_headers_.end()) {
        allowed_headers_.push_back(std::move(header));
    }

    return *this;
}

auto echo::middlewares::cors::allow_headers(
    std::vector<std::string> headers
) -> cors& {
    if (std::find(headers.begin(), headers.end(), "*") != headers.end()) {
        allow_header("*");
        return *this;
    }

    for (auto& header : headers) {
        allow_header(header);
    }

    return *this;
}

auto echo::middlewares::cors::expose_header(
    std::string header
) -> cors& {
    if (header == "*") {
        if (allow_credentials_) {
            throw std::invalid_argument("cannot expose wildcard headers when credentials are enabled");
        }

        any_exposed_header_ = true;
        exposed_headers_.clear();

        return *this;
    }

    if (any_exposed_header_) {
        any_exposed_header_ = false;
    }

    if (std::find(exposed_headers_.begin(), exposed_headers_.end(), header) == exposed_headers_.end()) {
        exposed_headers_.push_back(std::move(header));
    }

    return *this;
}

auto echo::middlewares::cors::expose_headers(
    std::vector<std::string> headers
) -> cors& {
    if (std::find(headers.begin(), headers.end(), "*") != headers.end()) {
        expose_header("*");
        return *this;
    }

    for (auto& header : headers) {
        expose_header(header);
    }

    return *this;
}

auto echo::middlewares::cors::allow_credentials(
    bool enabled
) -> cors& {
    if (enabled) {
        if (any_origin_ || any_method_ || any_header_ || any_exposed_header_) {
            throw std::invalid_argument("cannot enable credentials with wildcard policy");
        }
    }

    allow_credentials_ = enabled;

    return *this;
}

auto echo::middlewares::cors::max_age(
    std::uint32_t seconds
) -> cors& {
    max_age_ = seconds;

    return *this;
}

auto echo::middlewares::cors::handle(
    std::shared_ptr<echo::type::request> req,
    std::optional<echo::next_fn_t> next
) -> echo::awaitable<echo::type::response> {
    const auto request_origin = req->get_header("Origin");
    const auto request_method_header = req->get_header("Access-Control-Request-Method");
    const auto request_headers_header = req->get_header("Access-Control-Request-Headers");
    const bool is_preflight = req->method == "OPTIONS"
        && request_origin
        && request_method_header
        && !request_method_header->empty();

    echo::type::response response;
    if (is_preflight) {
        response = echo::type::response(200);
    } else if (next) {
        response = co_await (*next)(req);
    } else {
        response = echo::type::response(404);
    }

    if (!request_origin || request_origin->empty()) {
        co_return response;
    }

    const std::string origin = *request_origin;
    const bool origin_allowed = any_origin_
        || mirror_origin_
        || std::find(allowed_origins_.begin(), allowed_origins_.end(), origin) != allowed_origins_.end();

    if (!origin_allowed) {
        co_return response;
    }

    if (!is_preflight) {
        if (mirror_origin_) {
            response.set_header("Access-Control-Allow-Origin", origin);
        } else if (any_origin_) {
            response.set_header("Access-Control-Allow-Origin", "*");
        } else {
            response.set_header("Access-Control-Allow-Origin", origin);
        }

        if (allow_credentials_) {
            response.set_header("Access-Control-Allow-Credentials", "true");
        }

        if (any_exposed_header_) {
            response.set_header("Access-Control-Expose-Headers", "*");
        } else if (!exposed_headers_.empty()) {
            std::string joined;
            for (size_t index = 0; index < exposed_headers_.size(); ++index) {
                if (index > 0) {
                    joined += ", ";
                }

                joined += exposed_headers_[index];
            }

            response.set_header("Access-Control-Expose-Headers", joined);
        }

        {
            std::vector<std::string> tokens;

            if (const auto existing = response.headers.find("Vary"); existing != response.headers.end()) {
                std::string buffer;
                for (char ch : existing->second) {
                    if (ch == ',') {
                        size_t start = 0;
                        while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                            ++start;
                        }
                        size_t end = buffer.size();
                        while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                            --end;
                        }
                        if (start < end) {
                            tokens.push_back(buffer.substr(start, end - start));
                        }

                        buffer.clear();
                        continue;
                    }

                    buffer += ch;
                }

                if (!buffer.empty()) {
                    size_t start = 0;
                    while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                        ++start;
                    }
                    size_t end = buffer.size();
                    while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                        --end;
                    }
                    if (start < end) {
                        tokens.push_back(buffer.substr(start, end - start));
                    }
                }
            }

            for (const auto& token : { std::string("Origin") }) {
                bool found = false;
                for (const auto& existing_token : tokens) {
                    if (existing_token.size() != token.size()) {
                        continue;
                    }

                    bool equal = true;
                    for (size_t index = 0; index < token.size(); ++index) {
                        if (std::tolower(static_cast<unsigned char>(existing_token[index])) !=
                            std::tolower(static_cast<unsigned char>(token[index]))) {
                            equal = false;
                            break;
                        }
                    }

                    if (equal) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    tokens.push_back(token);
                }
            }

            if (!tokens.empty()) {
                std::string joined;
                for (size_t index = 0; index < tokens.size(); ++index) {
                    if (index > 0) {
                        joined += ", ";
                    }

                    joined += tokens[index];
                }

                response.set_header("Vary", joined);
            }
        }

        co_return response;
    }

    bool method_allowed = any_method_ || mirror_method_;
    if (!method_allowed && request_method_header) {
        const std::string requested_method = *request_method_header;
        for (const auto& method : allowed_methods_) {
            if (method() == requested_method) {
                method_allowed = true;
                break;
            }
        }
    }

    if (!method_allowed) {
        co_return response;
    }

    bool headers_allowed = true;
    if (request_headers_header && !request_headers_header->empty()) {
        if (!(any_header_ || mirror_request_headers_)) {
            std::string buffer;
            for (char ch : *request_headers_header) {
                if (ch == ',') {
                    size_t start = 0;
                    while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                        ++start;
                    }
                    size_t end = buffer.size();
                    while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                        --end;
                    }

                    if (start < end) {
                        std::string token = buffer.substr(start, end - start);
                        std::string lowered_token;
                        lowered_token.reserve(token.size());
                        for (char token_ch : token) {
                            lowered_token += static_cast<char>(std::tolower(static_cast<unsigned char>(token_ch)));
                        }

                        bool found = false;
                        for (const auto& allowed_header : allowed_headers_) {
                            if (allowed_header.size() != lowered_token.size()) {
                                continue;
                            }

                            bool match = true;
                            for (size_t index = 0; index < allowed_header.size(); ++index) {
                                if (std::tolower(static_cast<unsigned char>(allowed_header[index])) != lowered_token[index]) {
                                    match = false;
                                    break;
                                }
                            }

                            if (match) {
                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            headers_allowed = false;
                            break;
                        }
                    }

                    buffer.clear();
                    continue;
                }

                buffer += ch;
            }

            if (headers_allowed && !buffer.empty()) {
                size_t start = 0;
                while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                    ++start;
                }
                size_t end = buffer.size();
                while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                    --end;
                }

                if (start < end) {
                    std::string token = buffer.substr(start, end - start);
                    std::string lowered_token;
                    lowered_token.reserve(token.size());
                    for (char token_ch : token) {
                        lowered_token += static_cast<char>(std::tolower(static_cast<unsigned char>(token_ch)));
                    }

                    bool found = false;
                    for (const auto& allowed_header : allowed_headers_) {
                        if (allowed_header.size() != lowered_token.size()) {
                            continue;
                        }

                        bool match = true;
                        for (size_t index = 0; index < allowed_header.size(); ++index) {
                            if (std::tolower(static_cast<unsigned char>(allowed_header[index])) != lowered_token[index]) {
                                match = false;
                                break;
                            }
                        }

                        if (match) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        headers_allowed = false;
                    }
                }
            }
        }
    }

    if (!headers_allowed) {
        co_return response;
    }

    if (mirror_origin_) {
        response.set_header("Access-Control-Allow-Origin", origin);
    } else if (any_origin_) {
        response.set_header("Access-Control-Allow-Origin", "*");
    } else {
        response.set_header("Access-Control-Allow-Origin", origin);
    }

    if (allow_credentials_) {
        response.set_header("Access-Control-Allow-Credentials", "true");
    }

    std::string allow_methods_value;
    if (mirror_method_ && request_method_header) {
        allow_methods_value = *request_method_header;
    } else if (any_method_) {
        allow_methods_value = "*";
    } else {
        for (size_t index = 0; index < allowed_methods_.size(); ++index) {
            if (index > 0) {
                allow_methods_value += ", ";
            }

            allow_methods_value += allowed_methods_[index]();
        }
    }

    if (!allow_methods_value.empty()) {
        response.set_header("Access-Control-Allow-Methods", allow_methods_value);
    }

    if (mirror_request_headers_ && request_headers_header) {
        response.set_header("Access-Control-Allow-Headers", *request_headers_header);
    } else if (any_header_) {
        response.set_header("Access-Control-Allow-Headers", "*");
    } else if (!allowed_headers_.empty()) {
        std::string headers_value;
        for (size_t index = 0; index < allowed_headers_.size(); ++index) {
            if (index > 0) {
                headers_value += ", ";
            }

            headers_value += allowed_headers_[index];
        }

        response.set_header("Access-Control-Allow-Headers", headers_value);
    }

    if (max_age_) {
        response.set_header("Access-Control-Max-Age", std::to_string(*max_age_));
    }

    {
        std::vector<std::string> tokens;

        if (const auto existing = response.headers.find("Vary"); existing != response.headers.end()) {
            std::string buffer;
            for (char ch : existing->second) {
                if (ch == ',') {
                    size_t start = 0;
                    while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                        ++start;
                    }
                    size_t end = buffer.size();
                    while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                        --end;
                    }
                    if (start < end) {
                        tokens.push_back(buffer.substr(start, end - start));
                    }

                    buffer.clear();
                    continue;
                }

                buffer += ch;
            }

            if (!buffer.empty()) {
                size_t start = 0;
                while (start < buffer.size() && std::isspace(static_cast<unsigned char>(buffer[start]))) {
                    ++start;
                }
                size_t end = buffer.size();
                while (end > start && std::isspace(static_cast<unsigned char>(buffer[end - 1]))) {
                    --end;
                }
                if (start < end) {
                    tokens.push_back(buffer.substr(start, end - start));
                }
            }
        }

        for (const auto& token : { std::string("Origin"), std::string("Access-Control-Request-Method"), std::string("Access-Control-Request-Headers") }) {
            bool found = false;
            for (const auto& existing_token : tokens) {
                if (existing_token.size() != token.size()) {
                    continue;
                }

                bool equal = true;
                for (size_t index = 0; index < token.size(); ++index) {
                    if (std::tolower(static_cast<unsigned char>(existing_token[index])) !=
                        std::tolower(static_cast<unsigned char>(token[index]))) {
                        equal = false;
                        break;
                    }
                }

                if (equal) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                tokens.push_back(token);
            }
        }

        if (!tokens.empty()) {
            std::string joined;
            for (size_t index = 0; index < tokens.size(); ++index) {
                if (index > 0) {
                    joined += ", ";
                }

                joined += tokens[index];
            }

            response.set_header("Vary", joined);
        }
    }
    co_return response;
}
