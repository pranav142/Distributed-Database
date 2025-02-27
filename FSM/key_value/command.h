//
// Created by pknadimp on 2/15/25.
//

#ifndef COMMAND_H
#define COMMAND_H

#include <optional>
#include <string>
#include "FSM.h"
#include <nlohmann/json.hpp>

namespace kv {
    enum class RequestType {
        GET,
        SET,
        DELETE
    };

    struct Request {
        RequestType type;
        std::string key;
        std::string value;
    };

    struct Response {
        bool success;
        std::string data;
    };

    fsm::SerializedData serialize_request(const Request &request);

    std::optional<Request> deserialize_request(const fsm::SerializedData &serialized);

    fsm::SerializedData serialize_response(const Response &response);

    std::optional<Response> deserialize_response(const fsm::SerializedData &serialized);

    std::optional<RequestType> command_type_from_str(const std::string &str);

    std::string command_type_to_str(RequestType type);
}


#endif //COMMAND_H
