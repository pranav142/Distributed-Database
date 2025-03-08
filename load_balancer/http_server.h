//
// Created by pknadimp on 3/8/25.
//

#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "event.h"
#include "event_queue.h"
#include "crow.h"

namespace loadbalancer {
    class HTTPServer {
    public:
       explicit HTTPServer(utils::EventQueue<HTTPRequestEvent> &event_queue) : m_request_queue(event_queue) {
             setup();
       }

        void run(unsigned short port);

    private:
        void setup();

    private:
        crow::SimpleApp m_app;
        utils::EventQueue<HTTPRequestEvent> &m_request_queue;
    };
};

#endif //HTTP_SERVER_H
