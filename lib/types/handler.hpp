#ifndef ECHONEXUS_HANDLER_HPP
#define ECHONEXUS_HANDLER_HPP

#include <cstddef>
#include <functional>
#include <utility>

#include <boost/asio/awaitable.hpp>

#include "request.hpp"
#include "response.hpp"

namespace echo::type {

    using boost::asio::awaitable;

    /// @brief The handler that manages middleware and request handling.
    struct handler;

    /// @brief The type of the "next" function passed to middleware.
    ///
    /// Middleware receives a callable it can invoke to continue the chain.
    /// The callable returns an awaitable that yields a response.
    using next_fn_t = std::function<awaitable<response>()>;

    /// @brief The type of a middleware/handler function.
    ///
    /// A handler accepts a request reference and a next function, and returns
    /// an awaitable yielding a response.
    using handler_t = std::function<awaitable<response>(request &, next_fn_t)>;

    struct handler {
        /// @brief Default constructor.
        handler() = default;

        /// @brief Add a handler function to the chain.
        ///
        /// The provided handler is appended to the end of the middleware chain.
        ///
        /// @param h The handler function to add to the chain.
        void use(handler_t h) {
            chain.push_back(std::move(h));
        }

        void use(handler& h) {
            for (const auto& middleware : h.chain) {
                chain.push_back(middleware);
            }
        }

        /// @brief Set the fallback handler.
        void fallback(handler_t h) {
            fallback_ = std::move(h);
        }

        /// @brief Handle an incoming request by running the middleware chain.
        ///
        /// If no middleware is registered an empty/default response is returned.
        ///
        /// @param req The request to handle.
        /// @return An awaitable that yields the response.
        awaitable<response> handle(request &req) {
            if (chain.empty()) co_return response();
            co_return co_await dispatch(req, 0);
        }

    private:
        /// @brief The handler chain.
        ///
        /// Each element is a middleware function invoked in sequence.
        std::vector<handler_t> chain;

        /// @brief The fallback handler if no middleware is present.
        handler_t fallback_ = [](request &, next_fn_t) -> awaitable<response> {
            co_return response();
        };

        /// @brief Dispatch the request to the middleware at the given index.
        ///
        /// This function constructs the next callable that advances the chain and
        /// invokes the middleware at position index.
        ///
        /// @param req The request to forward to the middleware.
        /// @param index The index of the middleware to invoke.
        /// @return An awaitable yielding the response produced by the chain.
        awaitable<response> dispatch(request &req, size_t index) {
            if (index >= chain.size()) {
                if (fallback_ != nullptr) co_return co_await fallback_(req, []() -> awaitable<response> {
                    co_return response();
                });

                co_return response();
            }

            const auto& middleware = chain[index];

            if (middleware == nullptr) co_return response();

            const next_fn_t next = [this, &req, index]() -> awaitable<response> {
                co_return co_await this->dispatch(req, index + 1);
            };

            co_return co_await middleware(req, next);
        }
    };
}

#endif //ECHONEXUS_HANDLER_HPP
