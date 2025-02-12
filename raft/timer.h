//
// Created by pknadimp on 2/11/25.
//

#ifndef TIMER_H
#define TIMER_H

#include <boost/asio.hpp>

namespace raft {
    class Timer {
    public:
        explicit Timer(boost::asio::io_context &io)
            : m_io(io),
              m_timer(io) {
        }

        ~Timer() {
            m_timer.cancel();
        };

        void set(unsigned int duration_ms, std::function<void(const boost::system::error_code& ec)> callback);


        void cancel();

    private:
        boost::asio::io_context &m_io;
        boost::asio::steady_timer m_timer;
        std::function<void(const boost::system::error_code& ec)> m_callback;
    };
}

#endif //TIMER_H
