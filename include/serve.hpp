#ifndef ECHONEXUS_SERVE_HPP
#define ECHONEXUS_SERVE_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>

#include <boost/asio.hpp>

#ifndef DISABLE_BOOST_BEAST
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#endif

#include <handler.hpp>

namespace echo {
    namespace net = boost::asio;

    using net::awaitable;
    using net::ip::tcp;

    class executor {
    public:
        virtual awaitable<void> serve(const std::uint16_t port, next_fn_t handler) = 0;

        virtual ~executor() = default;
    };

#ifndef DISABLE_BOOST_BEAST
    namespace beast = boost::beast;
    namespace http  = beast::http;

    class beast_executor : public executor {
    private:
        std::shared_ptr<next_fn_t> handler_;

        http::response<http::string_body> to_beast_response(
            const type::response& res,
            unsigned version,
            bool keep_alive
        ) const;
        std::runtime_error make_socket_error(const char* action, const beast::error_code& ec) const;
        awaitable<http::response<http::string_body>> handle_request(
            const http::request<http::string_body>& req,
            const tcp::endpoint& remote
        );
        awaitable<void> session(tcp::socket socket);

    public:
        awaitable<void> serve(const std::uint16_t port, next_fn_t handler) override;
    };
#endif

    class nexus {
    private:
        std::unique_ptr<handler> handler_ = std::make_unique<handler>();

        std::unique_ptr<executor> exec;

    public:
        explicit nexus(std::unique_ptr<executor> exec);

        void use(const handler_t h);
        void use(std::shared_ptr<layer> layer);
        void use(const handler& h);
        void fallback(const handler_t h);

        /// @brief Start the server and listen for incoming connections on the specified port.
        ///
        /// @param port The port number to listen on for incoming HTTP requests.
        awaitable<void> serve(const std::uint16_t port);
    };
} // namespace echo

#endif // ECHONEXUS_SERVE_HPP
