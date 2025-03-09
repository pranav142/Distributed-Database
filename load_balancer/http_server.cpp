//
// Created by pknadimp on 3/8/25.
//

#include "http_server.h"
#include "serialized_data.h"

void loadbalancer::HTTPServer::run(unsigned short port) {
    m_app.port(port).multithreaded().run();
}

void loadbalancer::HTTPServer::shutdown() {
    m_app.stop();
}

void loadbalancer::HTTPServer::setup() {
    CROW_ROUTE(m_app, "/")([]() {
            return "HELLO WORLD";
        }
    );

    // Request Body Contains:
    //      key: <string>
    //      request: <string>
    CROW_ROUTE(m_app, "/request").methods(crow::HTTPMethod::POST)
    ([this](const crow::request &req) {
        std::condition_variable cv;
        bool processed = false;

        crow::json::wvalue http_response;

        // Check content type
        auto &content_type = req.get_header_value("Content-Type");
        if (content_type.find("application/x-www-form-urlencoded") == std::string::npos) {
            http_response["error_code"] = "INVALID_CONTENT_TYPE";
            http_response["response"] = "Expected application/x-www-form-urlencoded";
            return http_response;
        }

        auto body_params = req.get_body_params();

        std::string key = body_params.get("key");
        std::string request = body_params.get("request");

        LBClientRequest client_request;
        client_request.key = key;
        client_request.request = utils::serialized_data_from_string(request);

        HTTPRequestEvent event;

        event.request = client_request;
        event.callback = [&cv, &http_response, &processed](const LBClientResponse &response) {
            http_response["error_code"] = response.error_code_to_string();
            http_response["response"] = utils::serialized_data_to_string(response.response);
            processed = true;
            cv.notify_one();
        };

        m_request_queue.push(event); {
            std::mutex mtx;
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [&] { return processed; });
        }

        return http_response;
    });
}
