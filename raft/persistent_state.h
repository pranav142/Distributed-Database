//
// Created by pknadimp on 1/16/25.
//

#ifndef PERSISTENT_STATE_H
#define PERSISTENT_STATE_H

#include <string>
#include "logs.h"
#include "error_codes.h"
#include <optional>

namespace raft {
    // This string shows in a log file where all the header information such as term and vote is
    // This enables easy distinction between logs and header info
    const std::string HEADER_INDICATOR = "Header";

    class PersistentState {
    public:
        explicit PersistentState(std::string log_file_path);

        [[nodiscard]] unsigned int get_last_log_index() const;

        [[nodiscard]] unsigned int get_last_log_term() const;

        [[nodiscard]] ErrorCode append_log(const std::string &entry) const;


        [[nodiscard]] std::optional<Log> read_log(unsigned int index) const;

        [[nodiscard]] int get_voted_for() const;

        void set_voted_for(int candidate_id);

        [[nodiscard]] unsigned int get_current_term() const;

        void set_current_term(unsigned int term);

        void increment_term();

        [[nodiscard]] ErrorCode save_state() const;

        ErrorCode load_state();

        [[nodiscard]] unsigned int get_log_term(unsigned int index) const;

        [[nodiscard]] std::string get_entries_till_end(unsigned int index) const;

        void delete_logs(unsigned int index) const;

        void delete_log(unsigned int index);

        raft::ErrorCode add_entries(unsigned int start_index, const std::string &entries);

        [[nodiscard]] bool has_voted_for_no_one() const;

        void set_vote_for_no_one();

    private:
        [[nodiscard]] raft::ErrorCode append_log(const Log &log) const;

        unsigned int m_current_term = 0;

        // ID of candidate who was voted for; if -1 then no vote has been cast
        int m_voted_for = 0;
        std::string m_log_file_path;
    };
}


#endif //PERSISTENT_STATE_H
