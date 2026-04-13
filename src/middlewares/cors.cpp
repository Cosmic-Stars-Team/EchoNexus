#include <algorithm>
#include <middlewares/cors.hpp>
#include <stdexcept>

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

    return {};
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
    const auto exists = std::find_if(
        allowed_methods_.begin(),
        allowed_methods_.end(),
        [&](const allow_method& existing) {
            return existing() == method_name;
        }
    );

    if (exists == allowed_methods_.end()) {
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
    if (next) {
        co_return (*next)(req);
    }

    co_return echo::type::response(404);
}
