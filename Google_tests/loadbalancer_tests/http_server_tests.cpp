//
// Created by pknadimp on 3/8/25.
//

#include <gtest/gtest.h>

#include "http_server.h"
#include "event.h"

TEST(HttpServerTests, ServerRuns) {
    utils::EventQueue<loadbalancer::HTTPRequestEvent> m_request_queue;
    loadbalancer::HTTPServer server(m_request_queue);

    std::thread response_thread([&m_request_queue] (){
        while (true) {
            loadbalancer::HTTPRequestEvent event =  m_request_queue.pop();
            loadbalancer::LBClientResponse response;
            response.error_code = loadbalancer::LBClientResponse::ErrorCode::SUCCESS;
            event.callback(response);
        }
    });

    server.run(8080);

    if (response_thread.joinable()) {
        response_thread.join();
    }
}
