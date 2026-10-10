#pragma once

#include <atomic>
#include <memory>
#include <stop_token>

namespace Jerico
{
namespace Detail
{
struct CancellationState
{
    std::atomic_bool requested{false};
    std::stop_source stopSource;
};
}

class CancellationToken
{
public:
    [[nodiscard]] bool isCancellationRequested() const noexcept
    {
        const auto state = state_.lock();
        return state && state->requested.load(std::memory_order_acquire);
    }

    [[nodiscard]] std::stop_token stopToken() const noexcept
    {
        const auto state = state_.lock();
        return state ? state->stopSource.get_token() : std::stop_token{};
    }

private:
    explicit CancellationToken(std::weak_ptr<Detail::CancellationState> state)
        : state_(std::move(state))
    {
    }

    std::weak_ptr<Detail::CancellationState> state_;

    friend class CancellationSource;
};

class CancellationSource
{
public:
    CancellationSource()
        : state_(std::make_shared<Detail::CancellationState>())
    {
    }

    [[nodiscard]] CancellationToken token() const noexcept
    {
        return CancellationToken(state_);
    }

    void requestCancellation() noexcept
    {
        state_->requested.store(true, std::memory_order_release);
        state_->stopSource.request_stop();
    }

private:
    std::shared_ptr<Detail::CancellationState> state_;
};
}
