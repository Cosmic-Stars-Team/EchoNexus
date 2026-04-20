#include <middlewares/logger.hpp>
#include <middlewares/router.hpp>
#include <serve.hpp>
#include <types/request.hpp>
#include <types/response.hpp>

#include <chrono>
#include <exception>
#include <format>
#include <memory>
#include <optional>
#include <print>
#include <string>

auto global_start = std::chrono::high_resolution_clock::now();

namespace api {
    auto info(
        echo::type::request_ptr req,
        std::optional<echo::next_fn_t>
    ) -> echo::awaitable<echo::type::response> {
        auto now  = std::chrono::high_resolution_clock::now();
        auto diff = now - global_start;

        auto hours   = std::chrono::duration_cast<std::chrono::hours>(diff);
        auto minutes = std::chrono::duration_cast<std::chrono::minutes>(diff - hours);
        auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff - hours - minutes);
        auto ms      = std::chrono::duration_cast<std::chrono::milliseconds>(diff - hours - minutes - seconds);

        std::unordered_map<std::string, std::string> data = {
            {"name", "EchoNexus"},
            {"version", "0.0.0"},
            {"running", std::format("{}:{}:{}.{}", hours.count(), minutes.count(), seconds.count(), ms.count())},
        };

        co_return echo::type::response::json(data);
    }

    auto not_found(
        echo::type::request_ptr req,
        std::optional<echo::next_fn_t>
    ) -> echo::awaitable<echo::type::response> {
        std::unordered_map<std::string, std::string> data = {
            {"code", "404"},
            {"message", "Not Found"},
        };
        co_return echo::type::response::json(data, 404);
    }

    namespace v1 {
        auto user(
            echo::type::request_ptr req,
            std::optional<echo::next_fn_t>
        ) -> echo::awaitable<echo::type::response> {
            const auto* params = req->get_ctx<std::unordered_map<std::string, std::string>>("params");
            const auto* scope  = req->get_ctx<std::string>("scope");

            std::unordered_map<std::string, std::string> data = {
                {"route", "user"},
                {"uid", params == nullptr ? "missing" : params->at("uid")},
                {"scope", scope == nullptr ? "missing" : *scope},
            };

            co_return echo::type::response::json(data);
        }

        auto user_info(
            echo::type::request_ptr req,
            std::optional<echo::next_fn_t>
        ) -> echo::awaitable<echo::type::response> {
            const auto* params = req->get_ctx<std::unordered_map<std::string, std::string>>("params");

            std::unordered_map<std::string, std::string> data = {
                {"route", "info"},
                {"uid", params == nullptr ? "missing" : params->at("uid")},
            };

            co_return echo::type::response::json(data);
        }

        auto with_scope_layout(
            echo::type::request_ptr req,
            std::optional<echo::next_fn_t> next
        ) -> echo::awaitable<echo::type::response> {
            req->set_ctx("scope", std::string("user"));
            co_return co_await next.value()(req);
        }
    } // namespace v1
} // namespace api

auto main(
    void
) -> int {
#ifdef DISABLE_BOOST_BEAST
    std::println(stderr, "Boost.Beast is disabled. Rebuild without ECHONEXUS_DISABLE_BEAST to run this example.");
    return 1;
#else

    echo::net::io_context ioc(1);

    echo::nexus app(std::make_unique<echo::beast_executor>());

    echo::middlewares::router root;

    root.use(echo::middlewares::logger);

    echo::middlewares::router api;
    {
        echo::middlewares::router v1;
        {
            v1.get("/user/{uid}", api::v1::user).layer(api::v1::with_scope_layout);
            v1.get("/user/{uid}/info", api::v1::user_info);
        };

        api.nest("/v1", v1);
        api.get("/info", api::info);
        api.fallback(api::not_found);
    };

    root.get("/", [](echo::type::request_ptr, std::optional<echo::next_fn_t>) -> echo::awaitable<echo::type::response> {
        co_return echo::type::response::text("Hello, World!", 200);
    });
    root.nest("/api", api);

    app.use(std::make_shared<echo::middlewares::router>(root));
    app.fallback(
        [](std::shared_ptr<echo::type::request> req,
           std::optional<echo::next_fn_t>) -> echo::awaitable<echo::type::response> {
            co_return echo::type::response::text(std::string("No route for ") + req->path, 404);
        }
    );

    constexpr std::uint16_t port = 9000;
    std::println("EchoNexus example listening on http://127.0.0.1:{}", port);

    echo::net::co_spawn(ioc, app.serve(port), [&ioc](std::exception_ptr ep) {
        if (!ep) return;

        try {
            std::rethrow_exception(ep);
        } catch (const std::exception& e) {
            std::println(stderr, "Server error: {}", e.what());
        }

        ioc.stop();
    });

    ioc.run();

    return 0;
#endif
}
