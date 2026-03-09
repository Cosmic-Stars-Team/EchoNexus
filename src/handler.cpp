#include <handler.hpp>

namespace echo::type {
    awaitable<response> handler::dispatch(
        std::shared_ptr<state> st,
        std::shared_ptr<request> req,
        size_t index
    ) {
        if (index >= st->chain.size()) {
            if (st->fallback_ != nullptr) co_return co_await st->fallback_(req, std::nullopt);

            co_return response();
        }

        const auto& middleware = st->chain[index];

        if (middleware == nullptr) co_return co_await dispatch(st, req, index + 1);

        const next_fn_t next = [st, index](std::shared_ptr<request> next_req) -> awaitable<response> {
            co_return co_await dispatch(st, next_req, index + 1);
        };

        co_return co_await middleware(req, next);
    }

    void handler::use(
        handler_t h
    ) {
        state_->chain.push_back(std::move(h));
    }

    void handler::use(
        const handler& h
    ) {
        for (const auto& middleware : h.state_->chain) {
            state_->chain.push_back(middleware);
        }
    }

    void handler::fallback(
        handler_t h
    ) {
        state_->fallback_ = std::move(h);
    }

    awaitable<response> handler::handle(
        std::shared_ptr<request> req
    ) {
        if (state_->chain.empty()) {
            if (state_->fallback_ != nullptr) co_return co_await state_->fallback_(req, std::nullopt);

            co_return response();
        }

        co_return co_await dispatch(state_, req, 0);
    }
} // namespace echo::type
