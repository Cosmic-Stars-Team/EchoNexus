#ifndef ECHONEXUS_LIBRARY_H
#define ECHONEXUS_LIBRARY_H

#include "types/handler.hpp"
#include <boost/asio/awaitable.hpp>

namespace echo {
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
        boost::asio::awaitable<type::response> handle(std::shared_ptr<type::request> req);
    };
}

#endif // ECHONEXUS_LIBRARY_H
