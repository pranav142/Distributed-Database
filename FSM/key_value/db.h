//
// Created by pknadimp on 2/15/25.
//

#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include <mutex>
#include "command.h"
#include "FSM.h"

namespace kv {

    class DB final: public fsm::FSM {
    public:
        DB() = default;

        fsm::FSMResponse apply_command(const utils::SerializedData &serialized_command) override;

        fsm::FSMResponse query_state(const utils::SerializedData &serialized_query) override;

        std::optional<fsm::RequestType> get_request_type(const utils::SerializedData &serialized_request) override;
    private:
        Response process_command(const Request &request);

        Response process_query(const Request &request);

        std::optional<std::string> get_value(const std::string &key);
    private:
        std::mutex m_mtx;

        std::unordered_map<std::string, std::string> m_db;
    };
};


#endif //DB_H
