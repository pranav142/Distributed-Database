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

        fsm::FSMResponse apply_command(const fsm::SerializedData &serialized_command) override;

        fsm::FSMResponse query_state(const fsm::SerializedData &serialized_query) override;

        fsm::RequestType get_request_type(const fsm::SerializedData &serialized_request) override;

    private:
        kv::Response process_command(const Request &request);

    private:
        std::mutex m_mtx;

        std::unordered_map<std::string, std::string> m_db;
    };
};


#endif //DB_H
