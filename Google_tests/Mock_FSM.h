//
// Created by pknadimp on 2/26/25.
//

#ifndef MOCK_FSM_H
#define MOCK_FSM_H

#include "FSM.h"
#include "db.h"
#include <memory>

class MockFSM final : public raft::FSM {
public:
    raft::FSMResponse query_state(const std::string &serialized_command) override {
        auto command = db::deserialize_command(serialized_command);
        auto response = m_db->query_state(command.value());
        return raft::FSMResponse{response.success, response.data};
    }

    raft::FSMResponse apply_command(const std::string &serialized_command) override {
        auto command = db::deserialize_command(serialized_command);
        auto response = m_db->apply_command(command.value());
        return raft::FSMResponse{response.success, response.data};
    }

    bool is_modifying_command(const std::string &serialized_command) override {
        auto command = db::deserialize_command(serialized_command);
        return m_db->is_modifying_command(command.value());
    }

    raft::FSMResponse get_key_range(std::size_t lower_hash, std::size_t upper_hash) override {
        auto [success, keys] = m_db->get_key_range(lower_hash, upper_hash);

        std::string data;
        for (const auto &key : keys) {
            data += key + '\n';
        }

        return raft::FSMResponse{success, data};
    }

private:
    std::unique_ptr<db::DB> m_db = std::make_unique<db::DB>();
};


#endif //MOCK_FSM_H
