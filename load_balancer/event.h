#ifndef LOAD_BALANCER_EVENT_H_
#define LOAD_BALANCER_EVENT_H_

#include <functional>
#include <variant>
#include "serialized_data.h"

namespace loadbalancer {
    struct LBClientRequest {
        // Key for request routing
        std::string key;
        utils::SerializedData request;
    };

    struct LBClientResponse {
        enum class ErrorCode {
            SUCCESS,
            COULD_NOT_FIND_VALID_CLUSTER,
            INVALID_LEADER,
        } error_code;

        utils::SerializedData response;

        [[nodiscard]] std::string error_code_to_string() const {
            switch (error_code) {
                case ErrorCode::SUCCESS:
                    return "SUCCESS";
                case ErrorCode::COULD_NOT_FIND_VALID_CLUSTER:
                    return "COULD_NOT_FIND_VALID_CLUSTER";
                case ErrorCode::INVALID_LEADER:
                    return "INVALID_LEADER";
                default:
                    return "UNKNOWN_ERROR";
            }
        };
    };

    struct HTTPRequestEvent {
        LBClientRequest request;
        std::function<void(const LBClientResponse &)> callback;
    };

    struct QuitEvent {
    };

    typedef std::variant<HTTPRequestEvent, QuitEvent> RequestEvent;
}

#endif
