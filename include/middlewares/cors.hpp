#ifndef ECHONEXUS_MIDDLEWARES_CORS_HPP
#define ECHONEXUS_MIDDLEWARES_CORS_HPP

#include <boost/asio/awaitable.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <handler.hpp>
#include <types/request.hpp>
#include <types/response.hpp>

namespace echo::middlewares {

    struct allow_method {
        enum methods { GET, POS, PUT, PAT, DEL, OPT, HEA, TRC };

        constexpr allow_method(
            methods m
        ) noexcept : method_(m) {}

        auto operator()() const -> std::string;

    private:
        methods method_;
    };

    struct cors : public echo::layer {
        enum class preset { strict, permissive, development };

        cors();
        explicit cors(preset policy);

        auto allow_origin(std::string origin) -> cors&;
        auto allow_origins(std::vector<std::string> origins) -> cors&;
        auto allow_method(echo::middlewares::allow_method method) -> cors&;
        auto allow_methods(std::vector<echo::middlewares::allow_method> methods) -> cors&;
        auto allow_header(std::string header) -> cors&;
        auto allow_headers(std::vector<std::string> headers) -> cors&;
        auto expose_header(std::string header) -> cors&;
        auto expose_headers(std::vector<std::string> headers) -> cors&;
        auto allow_credentials(bool enabled) -> cors&;
        auto max_age(std::uint32_t seconds) -> cors&;

        auto handle(std::shared_ptr<echo::type::request>, std::optional<echo::next_fn_t>)
            -> echo::awaitable<echo::type::response> override;

    private:
        std::vector<std::string> allowed_origins_;
        std::vector<echo::middlewares::allow_method> allowed_methods_;
        std::vector<std::string> allowed_headers_;
        std::vector<std::string> exposed_headers_;
        bool any_origin_             = false;
        bool any_method_             = false;
        bool any_header_             = false;
        bool any_exposed_header_     = false;
        bool mirror_origin_          = false;
        bool mirror_method_          = false;
        bool mirror_request_headers_ = false;
        bool allow_credentials_      = false;
        std::optional<std::uint32_t> max_age_;
    };

} // namespace echo::middlewares

#endif // ECHONEXUS_MIDDLEWARES_CORS_HPP
