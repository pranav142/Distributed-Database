//
// Created by pknadimp on 2/15/25.
//

#ifndef FSM_H
#define FSM_H

#include <optional>
#include "serialized_data.h"

namespace fsm {
    enum class RequestType {
        QUERY,
        COMMAND
    };

    struct FSMResponse {
        enum class ErrorCode {
            SUCCESS,
            FAILED_TO_PARSE_REQUEST,
            FAILED_TO_PROCESS_REQUEST,
        } error_code;
        util::SerializedData serialized_response;
    };

    class FSM {
    public:
        virtual ~FSM() = default;

        // Applies a command to the FSM
        // Commands are requests that modify
        // the FSM state
        virtual FSMResponse apply_command(const util::SerializedData& serialized_command) = 0;

        // Applies a query to the FSM
        // Queries are requests that do not
        // modify the FSM state
        virtual FSMResponse query_state(const util::SerializedData& serialized_query) = 0;

        virtual std::optional<RequestType> get_request_type(const util::SerializedData& serialized_request) = 0;

        // TODO add functionality to turn requests into LOGs that can be appended

        // TODO add functionality to apply logs
    };
}


#endif //FSM_H
