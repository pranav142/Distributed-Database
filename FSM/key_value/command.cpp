//
// Created by pknadimp on 2/15/25.
//

#include "command.h"
#include <string>
#include <sstream>

utils::SerializedData kv::serialize_request(const Request &request) {
    // Format: TYPE|KEY|VALUE
    std::string serialized = command_type_to_str(request.type) + "|" +
                             request.key + "|" +
                             (request.value.empty() ? "null" : request.value);

    return {serialized.begin(), serialized.end()};
}

std::optional<kv::Request> kv::deserialize_request(const utils::SerializedData &serialized) {
    std::string data_str(serialized.begin(), serialized.end());

    std::vector<std::string> parts;
    size_t pos = 0;
    size_t prev_pos = 0;

    while ((pos = data_str.find('|', prev_pos)) != std::string::npos) {
        parts.push_back(data_str.substr(prev_pos, pos - prev_pos));
        prev_pos = pos + 1;
    }
    parts.push_back(data_str.substr(prev_pos));

    if (parts.size() != 3) {
        return std::nullopt;
    }

    std::optional<RequestType> type = command_type_from_str(parts[0]);
    if (!type.has_value()) {
        return std::nullopt;
    }

    std::string key = parts[1];
    std::string value = (parts[2] == "null") ? "" : parts[2];

    return Request{
        .type = type.value(),
        .key = key,
        .value = value
    };
}

utils::SerializedData kv::serialize_response(const Response &response) {
    // Format: SUCCESS|DATA
    std::string serialized = (std::to_string(response.success)) + "|" +
                            (response.data.empty() ? "null" : response.data);

    return {serialized.begin(), serialized.end()};
}

std::optional<kv::Response> kv::deserialize_response(const utils::SerializedData &serialized) {
    std::string data_str(serialized.begin(), serialized.end());

    size_t pos = data_str.find('|');
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string success_str = data_str.substr(0, pos);
    std::string data = data_str.substr(pos + 1);

    bool success = (success_str == "1");

    return Response{
        .success = success,
        .data = data
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
