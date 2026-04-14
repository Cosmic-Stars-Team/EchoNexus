#include <middlewares/router.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <utility>

namespace {
    [[nodiscard]] auto to_upper_copy(
        std::string value
    ) -> std::string {
        std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char ch) {
            return static_cast<char>(std::toupper(ch));
        });

        return value;
    }
} // namespace

struct echo::middlewares::router::mounted_child : echo::layer {
    std::string mount_prefix = "/";
    std::shared_ptr<state> child_state;

    auto handle(
        std::shared_ptr<echo::type::request> req,
        std::optional<echo::next_fn_t> next
    ) -> echo::awaitable<echo::type::response> override {
        const auto* parent_prefix_ptr = req->get_ctx<std::string>(router::prefix_ctx_key);
        const std::string parent_prefix = parent_prefix_ptr == nullptr ? std::string("/") : *parent_prefix_ptr;
        const std::string effective_prefix = router::join_prefix(parent_prefix, mount_prefix);

        if (!router::matches_prefix(req->path, effective_prefix)) {
            if (next.has_value()) co_return co_await next.value()(req);
            co_return echo::type::response(404);
        }

        const std::optional<std::string> saved_prefix =
            parent_prefix_ptr == nullptr ? std::nullopt : std::optional<std::string>(*parent_prefix_ptr);

        req->set_ctx(std::string(router::prefix_ctx_key), effective_prefix);

        auto restore_prefix = [&req, &saved_prefix]() {
            if (saved_prefix.has_value()) {
                req->set_ctx(std::string(router::prefix_ctx_key), saved_prefix.value());
            } else {
                req->context.erase(std::string(router::prefix_ctx_key));
            }
        };

        router child_view;
        child_view.state_ = child_state;

        try {
            echo::type::response res = co_await child_view.handle(req, std::nullopt);
            restore_prefix();
            co_return res;
        } catch (...) {
            restore_prefix();
            throw;
        }
    }
};

echo::middlewares::router::route_builder::route_builder(
    router& owner,
    std::string path
) : owner_(&owner), path_(router::normalize_path(std::move(path))) {}

auto echo::middlewares::router::route_builder::remember_selected(
    method route_method
) -> void {
    if (std::find(selected_methods_.begin(), selected_methods_.end(), route_method) != selected_methods_.end()) return;

    selected_methods_.push_back(route_method);
}

auto echo::middlewares::router::route_builder::register_single(
    method route_method,
    handler_t h
) -> route_builder& {
    if (reset_selection_on_next_registration_) {
        selected_methods_.clear();
        reset_selection_on_next_registration_ = false;
    }

    owner_->register_endpoint(path_, route_method, h);
    remember_selected(route_method);

    return *this;
}

auto echo::middlewares::router::route_builder::get(
    handler_t h
) -> route_builder& {
    return register_single(method::get, std::move(h));
}

auto echo::middlewares::router::route_builder::post(
    handler_t h
) -> route_builder& {
    return register_single(method::post, std::move(h));
}

auto echo::middlewares::router::route_builder::put(
    handler_t h
) -> route_builder& {
    return register_single(method::put, std::move(h));
}

auto echo::middlewares::router::route_builder::patch(
    handler_t h
) -> route_builder& {
    return register_single(method::patch, std::move(h));
}

auto echo::middlewares::router::route_builder::del(
    handler_t h
) -> route_builder& {
    return register_single(method::del, std::move(h));
}

auto echo::middlewares::router::route_builder::options(
    handler_t h
) -> route_builder& {
    return register_single(method::options, std::move(h));
}

auto echo::middlewares::router::route_builder::head(
    handler_t h
) -> route_builder& {
    return register_single(method::head, std::move(h));
}

auto echo::middlewares::router::route_builder::trace(
    handler_t h
) -> route_builder& {
    return register_single(method::trace, std::move(h));
}

auto echo::middlewares::router::route_builder::on(
    std::initializer_list<method> methods,
    handler_t h
) -> route_builder& {
    selected_methods_.clear();
    reset_selection_on_next_registration_ = false;

    for (const auto route_method : methods) {
        owner_->register_endpoint(path_, route_method, h);
        remember_selected(route_method);
    }

    return *this;
}

auto echo::middlewares::router::route_builder::layer(
    handler_t h
) -> route_builder& {
    for (const auto route_method : selected_methods_) {
        owner_->state_->routes[path_].endpoints[route_method].chain->use(h);
    }

    reset_selection_on_next_registration_ = true;
    return *this;
}

auto echo::middlewares::router::route_builder::layer(
    std::shared_ptr<echo::layer> layer
) -> route_builder& {
    return this->layer(
        [layer](std::shared_ptr<type::request> req, std::optional<next_fn_t> next) -> awaitable<type::response> {
            co_return co_await layer->handle(req, next);
        }
    );
}

auto echo::middlewares::router::route(
    std::string path
) -> route_builder {
    return route_builder(*this, std::move(path));
}

auto echo::middlewares::router::get(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::get(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.get(std::move(h));
    return builder;
}

auto echo::middlewares::router::post(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::post(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.post(std::move(h));
    return builder;
}

auto echo::middlewares::router::put(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::put(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.put(std::move(h));
    return builder;
}

auto echo::middlewares::router::patch(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::patch(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.patch(std::move(h));
    return builder;
}

auto echo::middlewares::router::del(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::del(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.del(std::move(h));
    return builder;
}

auto echo::middlewares::router::options(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::options(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.options(std::move(h));
    return builder;
}

auto echo::middlewares::router::head(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::head(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.head(std::move(h));
    return builder;
}

auto echo::middlewares::router::trace(
    std::string path
) -> route_builder {
    return route(std::move(path));
}

auto echo::middlewares::router::trace(
    std::string path,
    handler_t h
) -> route_builder {
    auto builder = route(std::move(path));
    builder.trace(std::move(h));
    return builder;
}

auto echo::middlewares::router::use(
    handler_t h
) -> router& {
    state_->pipeline.use(std::move(h));
    return *this;
}

auto echo::middlewares::router::use(
    std::shared_ptr<echo::layer> layer
) -> router& {
    state_->pipeline.use(std::move(layer));
    return *this;
}

auto echo::middlewares::router::use(
    const handler& h
) -> router& {
    state_->pipeline.use(h);
    return *this;
}

auto echo::middlewares::router::nest(
    std::string prefix,
    const router& child
) -> router& {
    auto mounted = std::make_shared<mounted_child>();
    mounted->mount_prefix = normalize_path(std::move(prefix));
    mounted->child_state  = child.state_;

    state_->pipeline.use(mounted);
    return *this;
}

auto echo::middlewares::router::handle(
    std::shared_ptr<echo::type::request> req,
    std::optional<echo::next_fn_t> next
) -> echo::awaitable<echo::type::response> {
    const std::string effective_prefix = current_prefix(*req);

    if (!matches_prefix(req->path, effective_prefix)) {
        if (next.has_value()) co_return co_await next.value()(req);
        co_return type::response(404);
    }

    co_return co_await state_->pipeline.handle_with_tail(
        req,
        route_tail_for(effective_prefix, std::move(next))
    );
}

auto echo::middlewares::router::normalize_path(
    std::string path
) -> std::string {
    if (path.empty()) return "/";
    if (path.front() != '/') return "/" + path;
    return path;
}

auto echo::middlewares::router::join_prefix(
    std::string_view parent,
    std::string_view child
) -> std::string {
    const std::string normalized_parent = normalize_path(std::string(parent));
    const std::string normalized_child  = normalize_path(std::string(child));

    if (normalized_parent == "/") return normalized_child;
    if (normalized_child == "/") return normalized_parent;

    return normalized_parent + normalized_child;
}

auto echo::middlewares::router::matches_prefix(
    std::string_view path,
    std::string_view prefix
) -> bool {
    const std::string normalized_path   = normalize_path(std::string(path));
    const std::string normalized_prefix = normalize_path(std::string(prefix));

    if (normalized_prefix == "/") return true;
    if (normalized_path == normalized_prefix) return true;
    if (normalized_path.size() <= normalized_prefix.size()) return false;
    if (!normalized_path.starts_with(normalized_prefix)) return false;

    return normalized_path[normalized_prefix.size()] == '/';
}

auto echo::middlewares::router::relative_path_for(
    std::string_view path,
    std::string_view prefix
) -> std::string {
    const std::string normalized_path   = normalize_path(std::string(path));
    const std::string normalized_prefix = normalize_path(std::string(prefix));

    if (normalized_prefix == "/") return normalized_path;
    if (normalized_path == normalized_prefix) return "/";

    return normalized_path.substr(normalized_prefix.size());
}

auto echo::middlewares::router::parse_method(
    const std::string& value
) -> std::optional<method> {
    const std::string upper = to_upper_copy(value);
    if (upper == "GET") return method::get;
    if (upper == "POST") return method::post;
    if (upper == "PUT") return method::put;
    if (upper == "PATCH") return method::patch;
    if (upper == "DELETE") return method::del;
    if (upper == "OPTIONS") return method::options;
    if (upper == "HEAD") return method::head;
    if (upper == "TRACE") return method::trace;

    return std::nullopt;
}

auto echo::middlewares::router::method_name(
    method m
) -> const char* {
    switch (m) {
    case method::get:
        return "GET";
    case method::post:
        return "POST";
    case method::put:
        return "PUT";
    case method::patch:
        return "PATCH";
    case method::del:
        return "DELETE";
    case method::options:
        return "OPTIONS";
    case method::head:
        return "HEAD";
    case method::trace:
        return "TRACE";
    }

    return "";
}

auto echo::middlewares::router::allow_header_value(
    const route_bucket& bucket
) -> std::string {
    static constexpr std::array<method, 8> method_order = {
        method::get, method::post, method::put, method::patch, method::del, method::options, method::head, method::trace
    };

    std::string allow;
    bool first = true;
    for (const auto route_method : method_order) {
        if (bucket.endpoints.find(route_method) == bucket.endpoints.end()) continue;

        if (!first) allow += ", ";
        allow += method_name(route_method);
        first = false;
    }

    return allow;
}

void echo::middlewares::router::register_endpoint(
    const std::string& path,
    method route_method,
    const handler_t& endpoint_handler
) {
    endpoint endpoint_entry;
    endpoint_entry.chain->fallback(endpoint_handler);

    state_->routes[normalize_path(path)].endpoints[route_method] = std::move(endpoint_entry);
}

auto echo::middlewares::router::current_prefix(
    const echo::type::request& req
) const -> std::string {
    const auto* prefix = req.get_ctx<std::string>(prefix_ctx_key);
    return prefix == nullptr ? std::string("/") : *prefix;
}

auto echo::middlewares::router::route_tail_for(
    std::string effective_prefix,
    std::optional<echo::next_fn_t> outer_next
) const -> echo::next_fn_t {
    return [st = state_, effective_prefix = std::move(effective_prefix), outer_next = std::move(outer_next)](
               std::shared_ptr<type::request> req
           ) -> awaitable<type::response> {
        const std::string relative_path = relative_path_for(req->path, effective_prefix);
        const auto route_it             = st->routes.find(relative_path);

        if (route_it == st->routes.end()) {
            if (outer_next.has_value()) co_return co_await outer_next.value()(req);
            co_return type::response(404);
        }

        const auto parsed_method = parse_method(req->method);
        if (!parsed_method.has_value()) {
            type::response res(405);
            res.set_header("Allow", allow_header_value(route_it->second));
            co_return res;
        }

        const auto endpoint_it = route_it->second.endpoints.find(parsed_method.value());
        if (endpoint_it == route_it->second.endpoints.end()) {
            type::response res(405);
            res.set_header("Allow", allow_header_value(route_it->second));
            co_return res;
        }

        co_return co_await endpoint_it->second.chain->handle(req);
    };
}
