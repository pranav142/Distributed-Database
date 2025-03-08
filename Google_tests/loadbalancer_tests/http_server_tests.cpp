//
// Created by pknadimp on 3/8/25.
//

#include <gtest/gtest.h>

#include "http_server.h"
#include "event.h"

// TODO: Currently Using Postman for API testing
// TODO: Switch to a more automated method
TEST(HttpServerTests, ServerRuns) {
    utils::EventQueue<loadbalancer::RequestEvent> m_request_queue;
    loadbalancer::HTTPServer server(m_request_queue);

    std::thread response_thread([&m_request_queue]() {
        bool is_running = true;
        while (is_running) {
            loadbalancer::RequestEvent event = m_request_queue.pop();
            std::visit([&event, &is_running](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, loadbalancer::QuitEvent>) {
                    is_running = false;
                } else if constexpr (std::is_same_v<T, loadbalancer::RequestEvent>) {
                    loadbalancer::LBClientResponse response;
                    response.error_code = loadbalancer::LBClientResponse::ErrorCode::SUCCESS;
                    arg.callback(response);
                }
            }, event);
        }
    });

    std::thread stop_thread([&server, &m_request_queue]() {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        server.shutdown();
        m_request_queue.push(loadbalancer::QuitEvent{});
    });

    server.run(8080);

    if (response_thread.joinable()) {
        response_thread.join();
    }

    if (stop_thread.joinable()) {
        stop_thread.join();
    }
}
