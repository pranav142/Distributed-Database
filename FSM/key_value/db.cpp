//
// Created by pknadimp on 2/15/25.
//

#include "db.h"

#include "command.h"

fsm::FSMResponse kv::DB::apply_command(const utils::SerializedData &serialized_command) {
    std::optional<Request> request = deserialize_request(serialized_command);
    if (!request.has_value()) {
        return fsm::FSMResponse{
            .error_code = fsm::FSMResponse::ErrorCode::FAILED_TO_PARSE_REQUEST,
        };
    }

    Response response = process_command(request.value());
    if (!response.success) {
        return fsm::FSMResponse{
            .error_code = fsm::FSMResponse::ErrorCode::FAILED_TO_PROCESS_REQUEST,
            .serialized_response = serialize_response(response),
        };
    }

    return fsm::FSMResponse{
        .error_code = fsm::FSMResponse::ErrorCode::SUCCESS,
        .serialized_response = serialize_response(response),
    };
}

fsm::FSMResponse kv::DB::query_state(const utils::SerializedData &serialized_query) {
    std::optional<Request> request = deserialize_request(serialized_query);
    if (!request.has_value()) {
        return fsm::FSMResponse{
            .error_code = fsm::FSMResponse::ErrorCode::FAILED_TO_PARSE_REQUEST,
        };
    }

    Response response = process_query(request.value());
    if (!response.success) {
        return fsm::FSMResponse{
            .error_code = fsm::FSMResponse::ErrorCode::FAILED_TO_PROCESS_REQUEST,
            .serialized_response = serialize_response(response),
        };
    }

    return fsm::FSMResponse{
        .error_code = fsm::FSMResponse::ErrorCode::SUCCESS,
        .serialized_response = serialize_response(response),
    };
}

std::optional<fsm::RequestType> kv::DB::get_request_type(const utils::SerializedData &serialized_request) {
    std::optional<Request> request = deserialize_request(serialized_request);
    if (!request.has_value()) {
        return std::nullopt;
    }

    switch (request->type) {
        case RequestType::GET:
            return fsm::RequestType::QUERY;
        case RequestType::SET:
        case RequestType::DELETE:
            return fsm::RequestType::COMMAND;
        default:
            return std::nullopt;
    }
}

bool kv::DB::is_valid_request(const utils::SerializedData &serialized_request) {
    std::optional<Request> req = deserialize_request(serialized_request);
    return req.has_value();
}

kv::Response kv::DB::process_command(const Request &request) {
    std::lock_guard lock(m_mtx);

    if (request.type == RequestType::SET) {
        m_db[request.key] = request.value;
        return Response{
            .success = true,
        };
    }

    if (request.type == RequestType::DELETE) {
        m_db.erase(request.key);
        return Response{
            .success = true,
        };
    }

    return Response{
        .success = false,
        .data = "Request Operation " + command_type_to_str(request.type) + " not supported"
    };
}

kv::Response kv::DB::process_query(const Request &request) {
    Response response;
    if (request.type != RequestType::GET) {
        response.success = false;
        response.data = "Command " + command_type_to_str(request.type) + " not supported for non modifying requests";
        return response;
    }

    std::string key = request.key;
    std::optional<std::string> value = get_value(key);
    if (!value.has_value()) {
        response.success = true;
        response.data = "null";
        return response;
    }

    response.success = true;
    response.data = value.value();
    return response;
}

std::optional<std::string> kv::DB::get_value(const std::string &key) {
    std::lock_guard lock(m_mtx);

    if (m_db.contains(key)) {
        return m_db.at(key);
    }

    return std::nullopt;
}
