//
// Created by pknadimp on 2/11/25.
//

#include "timer.h"

void utils::Timer::set(
    unsigned int duration_ms,
    std::function<void(const boost::system::error_code &error)> callback) {
    m_callback = std::move(callback);
    post(m_io, [duration_ms, this]() {
        m_timer.cancel();
        m_timer.expires_after(boost::asio::chrono::milliseconds(duration_ms));
        m_timer.async_wait(
            [this](const boost::system::error_code &error) { m_callback(error); });
    });
}

void utils::Timer::cancel() {
    post(m_io, [this]() { m_timer.cancel(); });
}
