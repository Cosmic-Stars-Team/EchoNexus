#include <serve.hpp>
#include <utils/parse.hpp>

#include <boost/system/system_error.hpp>

#include <stdexcept>
#include <string>

namespace echo {
#ifndef DISABLE_BOOST_BEAST
    http::response<http::string_body> beast_executor::to_beast_response(
        const type::response& res,
        const unsigned version,
        const bool keep_alive
    ) const {
        http::response<http::string_body> out{static_cast<http::status>(res.status), version};
        out.keep_alive(keep_alive);
        out.reason(res.message);
        out.body() = res.body;

        for (const auto& [key, value] : res.get_headers()) {
            out.set(key, value);
        }

        if (out.find(http::field::content_length) == out.end()) {
            out.content_length(out.body().size());
        }

        return out;
    }

    std::runtime_error beast_executor::make_socket_error(
        const char* action,
        const beast::error_code& ec
    ) const {
        return std::runtime_error(std::string(action) + ": " + ec.message());
    }

    awaitable<http::response<http::string_body>> beast_executor::handle_request(
        const http::request<http::string_body>& req,
        const tcp::endpoint& remote
    ) {
        if (handler_ == nullptr || !(*handler_)) {
            type::response no_handler(500);
            no_handler.set_content_type("text/plain; charset=utf-8");
            no_handler.set_body("No request handler configured.");
            co_return to_beast_response(no_handler, req.version(), req.keep_alive());
        }

        auto in = std::make_shared<type::request>();

        in->method       = std::string(req.method_string());
        in->target       = std::string(req.target());
        in->body         = req.body();
        in->http_version = static_cast<std::uint16_t>(req.version());

        // GCC 16 can misfire `-Wmaybe-uninitialized` here at `-O3` when this
        // return value is consumed through a structured binding.
        auto parsed_target = utils::parse_target(req.target());
        in->path           = std::move(parsed_target.first);
        in->query          = std::move(parsed_target.second);

        for (const auto& field : req) {
            in->headers[std::string(field.name_string())] = std::string(field.value());
        }

        if (!remote.address().is_unspecified()) {
            in->remote_addr = remote.address().to_string();
            in->remote_port = remote.port();
        }

        try {
            const type::response out = co_await (*handler_)(in);
            co_return to_beast_response(out, req.version(), req.keep_alive());
        } catch (const std::exception& e) {
            type::response err(500);
            err.set_content_type("text/plain; charset=utf-8");
            err.set_body(std::string("Unhandled exception: ") + e.what());
            co_return to_beast_response(err, req.version(), req.keep_alive());
        } catch (...) {
            type::response err(500);
            err.set_content_type("text/plain; charset=utf-8");
            err.set_body("Unhandled unknown exception.");
            co_return to_beast_response(err, req.version(), req.keep_alive());
        }
    }

    awaitable<void> beast_executor::session(
        tcp::socket socket
    ) {
        beast::flat_buffer buffer;

        beast::error_code remote_ec;
        const tcp::endpoint remote          = socket.remote_endpoint(remote_ec);
        const tcp::endpoint remote_or_empty = remote_ec ? tcp::endpoint{} : remote;

        for (;;) {
            http::request<http::string_body> req;

            beast::error_code read_ec;
            co_await http::async_read(socket, buffer, req, net::redirect_error(net::use_awaitable, read_ec));

            if (read_ec == http::error::end_of_stream) break;
            if (read_ec) break;

            http::response<http::string_body> res = co_await handle_request(req, remote_or_empty);
            const bool keep_alive                 = res.keep_alive();

            beast::error_code write_ec;
            co_await http::async_write(socket, res, net::redirect_error(net::use_awaitable, write_ec));

            if (write_ec) break;
            if (!keep_alive) break;
        }

        try {
            socket.shutdown(tcp::socket::shutdown_send);
        } catch (const boost::system::system_error&) {
            // Ignore shutdown errors on teardown.
        }
    }

    awaitable<void> beast_executor::serve(
        const std::uint16_t port,
        next_fn_t handler
    ) {
        if (!handler) {
            throw std::invalid_argument("Request handler cannot be empty.");
        }

        handler_ = std::make_shared<next_fn_t>(std::move(handler));

        const auto executor = co_await net::this_coro::executor;

        tcp::acceptor acceptor(executor);

        const auto run_or_throw = [this](auto&& op, const char* action) {
            try {
                op();
            } catch (const boost::system::system_error& e) {
                throw make_socket_error(action, e.code());
            }
        };

        run_or_throw([&acceptor] { acceptor.open(tcp::v4()); }, "acceptor.open failed");
        run_or_throw(
            [&acceptor] { acceptor.set_option(net::socket_base::reuse_address(true)); },
            "acceptor.set_option failed"
        );
        run_or_throw([&acceptor, port] { acceptor.bind(tcp::endpoint(tcp::v4(), port)); }, "acceptor.bind failed");
        run_or_throw(
            [&acceptor] { acceptor.listen(net::socket_base::max_listen_connections); },
            "acceptor.listen failed"
        );

        for (;;) {
            beast::error_code accept_ec;
            tcp::socket socket = co_await acceptor.async_accept(net::redirect_error(net::use_awaitable, accept_ec));

            if (accept_ec) continue;

            net::co_spawn(executor, session(std::move(socket)), net::detached);
        }
    }

#endif

    nexus::nexus(
        std::unique_ptr<executor> exec
    ) : exec(std::move(exec)) {
        if (this->exec == nullptr) {
            throw std::invalid_argument("Executor cannot be null");
        }
    }

    void nexus::use(
        const handler_t h
    ) {
        handler_->use(h);
    }

    void nexus::use(
        std::shared_ptr<layer> layer
    ) {
        handler_->use(std::move(layer));
    }

    void nexus::use(
        const handler& h
    ) {
        handler_->use(h);
    }

    void nexus::fallback(
        const handler_t h
    ) {
        handler_->fallback(h);
    }

    awaitable<void> nexus::serve(
        const std::uint16_t port
    ) {
        auto h = [this](std::shared_ptr<request> req) -> awaitable<response> {
            co_return co_await handler_->handle(req);
        };
        co_await exec->serve(port, std::move(h));
    }
} // namespace echo
