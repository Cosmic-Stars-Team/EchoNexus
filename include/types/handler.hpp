#ifndef ECHONEXUS_TYPES_HANDLER_HPP
#define ECHONEXUS_TYPES_HANDLER_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

#include <boost/asio/awaitable.hpp>

#include "request.hpp"
#include "response.hpp"

namespace echo::type {
    using boost::asio::awaitable;

    /// @brief The type of the "next" function passed to middleware.
    ///
    /// Middleware receives a callable it can invoke to continue the chain.
    /// The callable accepts a shared request object and returns an awaitable
    /// that yields a response.
    using next_fn_t = std::function<awaitable<response>(std::shared_ptr<request>)>;

    /// @brief The type of a middleware/handler function.
    ///
    /// A handler accepts a shared request object and an optional next function.
    /// If present, next continues the middleware chain.
    /// The handler returns an awaitable yielding a response.
    using handler_t = std::function<awaitable<response>(std::shared_ptr<request>, std::optional<next_fn_t>)>;

    /// @brief The handler that manages middleware and request handling.
    class handler {
    private:
        struct state {
            /// @brief The handler chain.
            ///
            /// Each element is a middleware function invoked in sequence.
            std::vector<handler_t> chain;

            /// @brief The fallback handler used when the chain is exhausted.
            handler_t fallback_ = nullptr;
        };

        std::shared_ptr<state> state_ = std::make_shared<state>();

        /// @brief Dispatch the request to the middleware at the given index.
        ///
        /// This function constructs the next callable that advances the chain and
        /// invokes the middleware at position index.
        ///
        /// @param req The shared request object to forward to the middleware.
        /// @param index The index of the middleware to invoke.
        /// @return An awaitable yielding the response produced by the chain.
        static awaitable<response> dispatch(std::shared_ptr<state> st, std::shared_ptr<request> req, size_t index);

    public:
        /// @brief Default constructor.
        handler()                                = default;
        handler(const handler& other)            = delete;
        handler& operator=(const handler& other) = delete;

        /// @brief Add a handler function to the chain.
        ///
        /// The provided handler is appended to the end of the middleware chain.
        ///
        /// @param h The handler function to add to the chain.
        void use(handler_t h);

        /// @brief Add all handlers from another handler's chain to this handler.
        ///
        /// The handlers from the provided handler are appended to the end of this
        /// handler's chain.
        ///
        /// @param h The handler whose chain of middleware functions should be added
        /// to this handler.
        void use(const handler& h);

        /// @brief Set the fallback handler.
        ///
        /// The fallback handler is invoked when the middleware chain is exhausted,
        /// including the case where no middleware is registered.
        ///
        /// @param h The handler function to set as the fallback.
        void fallback(handler_t h);

        /// @brief Handle an incoming request by running the middleware chain.
        ///
        /// If no middleware is registered, fallback runs first when set;
        /// otherwise an empty/default response is returned.
        ///
        /// @param req The shared request object to handle.
        /// @return An awaitable that yields the response.
        awaitable<response> handle(std::shared_ptr<request> req);
    };
} // namespace echo::type

#endif // ECHONEXUS_TYPES_HANDLER_HPP
