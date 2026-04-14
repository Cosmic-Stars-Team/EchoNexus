#ifndef ECHONEXUS_MIDDLEWARES_ROUTER_HPP
#define ECHONEXUS_MIDDLEWARES_ROUTER_HPP

#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <handler.hpp>

namespace echo::middlewares {
    struct router : public echo::layer {
        enum class method { get, post, put, patch, del, options, head, trace };

        struct route_builder {
            route_builder(router& owner, std::string path);

            auto get(handler_t h) -> route_builder&;
            auto post(handler_t h) -> route_builder&;
            auto put(handler_t h) -> route_builder&;
            auto patch(handler_t h) -> route_builder&;
            auto del(handler_t h) -> route_builder&;
            auto options(handler_t h) -> route_builder&;
            auto head(handler_t h) -> route_builder&;
            auto trace(handler_t h) -> route_builder&;

            auto on(std::initializer_list<method> methods, handler_t h) -> route_builder&;
            auto layer(handler_t h) -> route_builder&;
            auto layer(std::shared_ptr<echo::layer> layer) -> route_builder&;

        private:
            router* owner_;
            std::string path_;
            std::vector<method> selected_methods_;
            bool reset_selection_on_next_registration_ = false;

            auto register_single(method route_method, handler_t h) -> route_builder&;
            void remember_selected(method route_method);
        };

        router() = default;

        auto route(std::string path) -> route_builder;

        auto get(std::string path) -> route_builder;
        auto get(std::string path, handler_t h) -> route_builder;
        auto post(std::string path) -> route_builder;
        auto post(std::string path, handler_t h) -> route_builder;
        auto put(std::string path) -> route_builder;
        auto put(std::string path, handler_t h) -> route_builder;
        auto patch(std::string path) -> route_builder;
        auto patch(std::string path, handler_t h) -> route_builder;
        auto del(std::string path) -> route_builder;
        auto del(std::string path, handler_t h) -> route_builder;
        auto options(std::string path) -> route_builder;
        auto options(std::string path, handler_t h) -> route_builder;
        auto head(std::string path) -> route_builder;
        auto head(std::string path, handler_t h) -> route_builder;
        auto trace(std::string path) -> route_builder;
        auto trace(std::string path, handler_t h) -> route_builder;

        auto use(handler_t h) -> router&;
        auto use(std::shared_ptr<echo::layer> layer) -> router&;
        auto use(const handler& h) -> router&;
        auto nest(std::string prefix, const router& child) -> router&;

        auto handle(std::shared_ptr<echo::type::request> req, std::optional<echo::next_fn_t> next)
            -> echo::awaitable<echo::type::response> override;

    private:
        struct mounted_child;

        struct method_hash {
            auto operator()(method m) const noexcept -> std::size_t {
                return static_cast<std::size_t>(m);
            }
        };

        struct endpoint {
            std::shared_ptr<handler> chain = std::make_shared<handler>();
        };

        struct route_bucket {
            std::unordered_map<method, endpoint, method_hash> endpoints;
        };

        struct state {
            handler pipeline;
            std::unordered_map<std::string, route_bucket> routes;
        };

        friend struct mounted_child;

        static constexpr std::string_view prefix_ctx_key = "__echo.router.prefix";

        std::shared_ptr<state> state_ = std::make_shared<state>();

        static auto normalize_path(std::string path) -> std::string;
        static auto join_prefix(std::string_view parent, std::string_view child) -> std::string;
        static auto matches_prefix(std::string_view path, std::string_view prefix) -> bool;
        static auto relative_path_for(std::string_view path, std::string_view prefix) -> std::string;
        static auto parse_method(const std::string& value) -> std::optional<method>;
        static auto method_name(method m) -> const char*;
        static auto allow_header_value(const route_bucket& bucket) -> std::string;

        void register_endpoint(
            const std::string& path,
            method route_method,
            const handler_t& endpoint_handler
        );

        auto current_prefix(const echo::type::request& req) const -> std::string;
        auto route_tail_for(std::string effective_prefix, std::optional<echo::next_fn_t> outer_next) const
            -> echo::next_fn_t;
    };
} // namespace echo::middlewares

#endif // ECHONEXUS_MIDDLEWARES_ROUTER_HPP
