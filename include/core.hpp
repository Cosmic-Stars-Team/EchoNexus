#ifndef ECHONEXUS_CORE_HPP
#define ECHONEXUS_CORE_HPP

#include <boost/asio/awaitable.hpp>

#include <types/handler.hpp>
#include <types/request.hpp>
#include <types/response.hpp>

namespace echo {
    using boost::asio::awaitable;

    /// @brief EchoNexus initializes the server and orchestrates incoming HTTP request processing.
    class nexus {
    private:
        /// @brief The handler instance that manages the middleware chain.
        std::unique_ptr<type::handler> handler;

    public:
        nexus();

        /// @brief The handle function of EchoNexus
        ///
        /// @param req - The incoming HTTP request to be processed.
        awaitable<type::response> handle(std::shared_ptr<type::request> req);
    };
} // namespace echo

#endif // ECHONEXUS_CORE_HPP
