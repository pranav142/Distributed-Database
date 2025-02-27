//
// Created by pknadimp on 2/15/25.
//

#include "command.h"
#include <string>
#include <sstream>

fsm::SerializedData kv::serialize_request(const Request &request) {
    using json = nlohmann::json;
    json j;

    j["type"] = command_type_to_str(request.type);
    j["key"] = request.key;
    j["value"] = request.value.empty() ? "null" : request.value;

    return json::to_ubjson(j);
}

std::optional<kv::Request> kv::deserialize_request(const fsm::SerializedData &serialized) {
    using json = nlohmann::json;
    json j = json::from_ubjson(serialized);

    if (!j.contains("type") || !j.contains("key") || !j.contains("value")) {
        return std::nullopt;
    }

    std::optional<RequestType> type = command_type_from_str(j.at("type"));
    if (!type.has_value()) {
        return std::nullopt;
    }

    return Request{
        .type = type.value(),
        .key = j.at("key"),
        .value = j.at("value")
    };
}

fsm::SerializedData kv::serialize_response(const Response &response) {
    using json = nlohmann::json;
    json j;

    j["data"] = response.data.empty() ? "null" : response.data;
    j["success"] = response.success;

    return json::to_ubjson(j);
}

std::optional<kv::Response> kv::deserialize_response(const fsm::SerializedData &serialized) {
    using json = nlohmann::json;
    json j = json::from_ubjson(serialized);

    if (!j.contains("data") || !j.contains("success")) {
        return std::nullopt;
    }

    return Response{
        .success = j.at("success"),
        .data = j.at("data"),
    };
}

std::optional<kv::RequestType> kv::command_type_from_str(const std::string &str) {
    switch (str[0]) {
        case 'G':
            return RequestType::GET;
        case 'D':
            return RequestType::DELETE;
        case 'S':
            return RequestType::SET;
        default:
            return std::nullopt;
    }
}

std::string kv::command_type_to_str(RequestType type) {
    switch (type) {
        case RequestType::GET:
            return "GET";
        case RequestType::DELETE:
            return "DELETE";
        case RequestType::SET:
            return "SET";
        default:
            return "UNKNOWN";
    }
}
