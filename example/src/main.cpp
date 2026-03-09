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

    app.use(
        [](std::shared_ptr<echo::type::request> req,
           std::optional<echo::next_fn_t> next) -> echo::awaitable<echo::type::response> {
            echo::type::response res(200);
            req->set_ctx("test-context", "Hello, EchoNexus");
            if (next) {
                res = co_await next.value()(req);
            }
            co_return res;
        }
    );

    app.fallback(
        [](std::shared_ptr<echo::type::request> req,
           std::optional<echo::next_fn_t> _next) -> echo::awaitable<echo::type::response> {
            echo::type::response res(200);
            res.set_content_type("text/plain; charset=utf-8");
            const auto content = req->get_ctx<const char*>(std::string_view("test-context"));
            if (content) {
                res.set_body(*content);
            } else {
                res.set_body("Hello, World!");
            }
            co_return res;
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
