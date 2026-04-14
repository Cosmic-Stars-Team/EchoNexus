#include <middlewares/logger.hpp>
#include <middlewares/router.hpp>
#include <serve.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <print>
#include <string>

int main() {
#ifdef DISABLE_BOOST_BEAST
    std::println(stderr, "Boost.Beast is disabled. Rebuild without ECHONEXUS_DISABLE_BEAST to run this example.");
    return 1;
#else

    echo::net::io_context ioc(1);

    echo::nexus app(std::make_unique<echo::beast_executor>());

    auto root = std::make_shared<echo::middlewares::router>();
    echo::middlewares::router api;
    echo::middlewares::router v1;

    root->use(echo::middlewares::logger);

    v1.get(
          "/user",
          [](std::shared_ptr<echo::type::request> req, std::optional<echo::next_fn_t>)
              -> echo::awaitable<echo::type::response> {
              const auto* scope = req->get_ctx<std::string>("scope");
              const std::string scope_value = scope == nullptr ? "missing" : *scope;
              co_return echo::type::response::json(std::string(R"({"scope":")") + scope_value + R"("})", 200);
          }
      )
        .layer([](std::shared_ptr<echo::type::request> req, std::optional<echo::next_fn_t> next)
                   -> echo::awaitable<echo::type::response> {
            req->set_ctx("scope", std::string("v1-user"));
            co_return co_await next.value()(req);
        });

    api.get(
        "/",
        [](std::shared_ptr<echo::type::request>, std::optional<echo::next_fn_t>) -> echo::awaitable<echo::type::response> {
            co_return echo::type::response::text("api root", 200);
        }
    );
    api.nest("/v1", v1);

    root->get(
        "/",
        [](std::shared_ptr<echo::type::request>, std::optional<echo::next_fn_t>) -> echo::awaitable<echo::type::response> {
            co_return echo::type::response::text("EchoNexus", 200);
        }
    );
    root->nest("/api", api);

    app.use(root);
    app.fallback([](std::shared_ptr<echo::type::request> req, std::optional<echo::next_fn_t>) -> echo::awaitable<echo::type::response> {
        co_return echo::type::response::text(std::string("No route for ") + req->path, 404);
    });

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
