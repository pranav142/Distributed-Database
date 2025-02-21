//
// Created by pknadimp on 1/18/25.

#include "node.h"

#include <thread>

#include "gRPC_server.h"
#include "utils.h"

raft::ServerState raft::Node::get_server_state() const {
    return m_server_state;
}

unsigned int raft::Node::get_id() const {
    return m_id;
}

unsigned int raft::Node::get_commit_index() const {
    return m_commit_index;
}

unsigned int raft::Node::get_last_applied_index() const {
    return m_last_applied_index;
}

unsigned int raft::Node::get_current_term() const {
    return m_state.get_current_term();
}

void raft::Node::set_current_term(unsigned int term) {
    m_state.set_current_term(term);
}

void raft::Node::on_election_timeout(const boost::system::error_code &ec) {
    if (!ec) {
        m_event_queue.push(ElectionTimeoutEvent{});
    } else if (ec != boost::asio::error::operation_aborted) {
        m_logger->error("ElectionTimeout error: {}", ec.message());
    }
}

void raft::Node::reset_election_timer() {
    unsigned int random_time_ms = generate_random_number(m_timer_settings.election_timer_min_ms,
                                                         m_timer_settings.election_timer_max_ms);
    m_election_timer.set(random_time_ms, [this](const boost::system::error_code &ec) {
        on_election_timeout(ec);
    });
}


void raft::Node::on_heartbeat_timeout(const boost::system::error_code &ec) {
    if (!ec) {
        m_event_queue.push(HeartBeatEvent{});
    } else if (ec != boost::asio::error::operation_aborted) {
        m_logger->error("Heartbeat timeout error: {}", ec.message());
    }
}

void raft::Node::reset_heartbeat_timer() {
    m_heartbeat_timer.set(m_timer_settings.heart_beat_interval_ms, [this](const boost::system::error_code &ec) {
        on_heartbeat_timeout(ec);
    });
}

void raft::Node::shut_down_heartbeat_timer() {
    m_heartbeat_timer.cancel();
}

auto raft::Node::on_lease_timeout(const boost::system::error_code &ec) {
    if (!ec) {
        m_event_queue.push(LeaseTimeoutEvent{});
    } else if (ec != boost::asio::error::operation_aborted) {
        m_logger->error("Lease timeout error: {}", ec.message());
    }
}

void raft::Node::reset_lease_timer() {
    m_lease_timer.set(m_timer_settings.lease_timer_ms, [this](const boost::system::error_code &ec) {
        on_lease_timeout(ec);
    });
}

void raft::Node::shut_down_lease_timer() {
    m_lease_timer.cancel();
}

void raft::Node::log_current_state() const {
    m_logger->debug("State: {}, term: {}", server_state_to_str(m_server_state), m_state.get_current_term());
}

void raft::Node::initialize() {
    if (m_server_state == ServerState::LEADER) {
        reset_heartbeat_timer();
    } else {
        reset_election_timer();
    }
}

void raft::Node::shut_down_election_timer() {
    m_election_timer.cancel();
}

void raft::Node::cancel() {
    m_event_queue.push(QuitEvent{});
}

void raft::Node::stop() {
    m_running = false;
    m_server.stop();
    shut_down_election_timer();
    shut_down_heartbeat_timer();
    m_work_guard.reset();
    m_io.stop();
}

void raft::Node::become_follower(unsigned int term) {
    m_server_state = ServerState::FOLLOWER;

    m_leader_id = -1;
    // Reset election timer and shut down heartbeat timer when becoming a follower.
    reset_election_timer();
    shut_down_heartbeat_timer();
    shut_down_lease_timer();
    m_state.set_current_term(term);
    m_state.set_vote_for_no_one();
    log_current_state();
    clear_pending_requests();
}

void raft::Node::become_candidate() {
    m_server_state = ServerState::CANDIDATE;

    m_leader_id = -1;
    m_state.increment_term();
    shut_down_heartbeat_timer();
    shut_down_lease_timer();
    m_state.set_voted_for(static_cast<int>(m_id));
    reset_election_timer();
    log_current_state();
    clear_pending_requests();
}

void raft::Node::become_leader() {
    m_server_state = ServerState::LEADER;

    // No more election timeouts when leader
    m_leader_id = static_cast<int>(m_id);
    shut_down_election_timer();
    reset_heartbeat_timer();
    reset_lease_timer();
    m_state.set_vote_for_no_one();
    log_current_state();
}

void raft::Node::run_follower_loop() {
    bool should_exit = false;
    while (m_server_state == ServerState::FOLLOWER && !should_exit) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeoutEvent>) {
                m_logger->debug("ElectionTimeoutEvent occurred");
                become_candidate();
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                stop();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                // the term of request is lower, deny it
                m_logger->debug("Received Append Entries request from: {}", arg.leader_id);
                if (arg.term < m_state.get_current_term()) {
                    AppendEntriesResponse response{};
                    response.success = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    arg.callback(response);
                    m_logger->debug(
                        "Denying Append Entries Request because term of request {} is less than our term {}", arg.term,
                        m_state.get_current_term());
                    return;
                }

                // if the term is higher, adjust the term and set vote for no one
                if (arg.term > m_state.get_current_term()) {
                    m_state.set_current_term(arg.term);
                    m_state.set_vote_for_no_one();
                }

                reset_election_timer();
                m_leader_id = arg.leader_id;
                // this checks if there is not a match
                // if the previous log index is greater than any index we have -> deny
                // if the log term of the index is mismatched -> deny
                if (arg.prev_log_index > m_state.get_last_log_index() || (arg.prev_log_term != m_state.get_log_term(
                                                                              arg.prev_log_index) && arg.prev_log_index
                                                                          != 0)) {
                    m_logger->debug("Denying append entries because could not find matching entry {}",
                                    arg.prev_log_index);
                    AppendEntriesResponse response{};
                    response.success = false;
                    response.term = m_state.get_current_term();
                    arg.callback(response);
                    return;
                }

                if (m_state.add_entries(arg.prev_log_index + 1, arg.entries) != SUCCESS) {
                    AppendEntriesResponse response{};
                    response.success = false;
                    response.term = m_state.get_current_term();
                    arg.callback(response);
                    return;
                }

                update_commit_index(arg.commit_index);

                AppendEntriesResponse response{};
                response.success = true;
                response.term = m_state.get_current_term();
                arg.callback(response);
            } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                m_logger->warn("AppendEntriesResponseEvent received in Follower state");
            } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                m_logger->warn("HeartbeatEvent ignored in Follower state");
            } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                m_logger->debug("Received RequestVoteResponse: vote_granted={}, term={}", arg.vote_granted, arg.term);
                // If the response has a higher term, update our term and reset our vote.
                if (arg.term > m_state.get_current_term()) {
                    m_state.set_current_term(arg.term);
                    m_state.set_vote_for_no_one();
                }
            } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                m_logger->debug("Received RequestVote from candidate: {}", arg.candidate_id);
                // If the request has a lower term, immediately deny the vote.
                if (arg.term < m_state.get_current_term()) {
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = m_state.get_current_term();
                    arg.callback(response);
                    m_logger->debug("Denying vote to {} because candidate term ({}) is less than our term ({})",
                                    arg.candidate_id, arg.term, m_state.get_current_term());
                    return;
                }

                // If a valid RPC is sent, prevent unnecessary election.
                reset_election_timer();

                // If the request has a higher term, update our term and indicate we haven't cast a vote.
                if (arg.term > m_state.get_current_term()) {
                    m_state.set_current_term(arg.term);
                    m_state.set_voted_for(-1);
                }

                RequestVoteResponse response{};
                response.vote_granted = false;
                response.term = static_cast<int>(m_state.get_current_term());

                // If no vote has been cast or already voted for this candidate,
                // only grant vote if the candidate's log is as up-to-date or more.
                if ((m_state.has_voted_for_no_one() || m_state.get_voted_for() == arg.candidate_id) &&
                    is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                    response.vote_granted = true;
                    m_state.set_voted_for(arg.candidate_id);
                    m_logger->debug("Granting vote to candidate: {}", arg.candidate_id);
                    arg.callback(response);
                    return;
                }

                arg.callback(response);
                m_logger->debug("Denying vote to candidate {} because candidate's log is not up-to-date",
                                arg.candidate_id);
            } else if constexpr (std::is_same_v<T, ClientRequestEvent>) {
                m_logger->critical("Have not implemented Client Request Handling");
                ClientRequestResponse response{};
                response.success = false;
                response.redirect = true;
                response.leader_id = m_leader_id;
                arg.callback(response);
            }
        }, event);
    }
}

void raft::Node::request_vote(const std::string &address) {
    RequestVoteRPC request_vote_rpc{};
    request_vote_rpc.candidate_id = m_id;
    request_vote_rpc.term = m_state.get_current_term();
    request_vote_rpc.last_log_index = m_state.get_last_log_index();
    request_vote_rpc.last_log_term = m_state.get_last_log_term();

    m_client->request_vote(address, request_vote_rpc, [this, address](const RequestVoteResponse &response) {
        if (response.error) {
            m_logger->error("RequestVote failed for address {}: node may be offline", address);
            return;
        }
        RequestVoteResponseEvent request_vote_response_event{};
        request_vote_response_event.term = response.term;
        request_vote_response_event.vote_granted = response.vote_granted;
        m_event_queue.push(request_vote_response_event);
    });
}

void raft::Node::start_election() {
    for (auto &pair: m_cluster) {
        // skip sending vote to self
        if (pair.first == m_id) {
            continue;
        }
        request_vote(pair.second.address);
    }
}

void raft::Node::run_candidate_loop() {
    // set to one since we vote for our self
    int votes_granted = 1;
    int quorum = calculate_quorum();

    start_election();

    bool should_exit = false;
    while (m_server_state == ServerState::CANDIDATE) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit, &votes_granted, &quorum](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeoutEvent>) {
                m_logger->debug("Election timeout received");
                become_candidate();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                m_logger->debug("Received quit signal");
                should_exit = true;
                stop();
            } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                m_logger->debug("Received AppendEntries from leader: {}", arg.leader_id);
                if (arg.term < m_state.get_current_term()) {
                    AppendEntriesResponse response{};
                    response.term = m_state.get_current_term();
                    response.success = false;
                    arg.callback(response);
                    m_logger->debug("Denying AppendEntries from {}: leader's term ({}) is less than our term ({})",
                                    arg.leader_id, arg.term, m_state.get_current_term());
                    return;
                }

                m_logger->debug("Recognizing new leader and stepping down");
                become_follower(arg.term);
                // push the event to be processed in the follower loop
                m_event_queue.push(arg);
            } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                m_logger->warn("Received AppendEntriesResponse as Candidate");
            } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                m_logger->warn("Received HeartBeatEvent in Candidate state");
            } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                m_logger->debug("Received vote: vote_granted={}, term={}", arg.vote_granted, arg.term);
                // term from voter cannot be less than candidate's term unless it’s from a previous election.
                if (arg.term < m_state.get_current_term()) {
                    m_logger->warn("Received an outdated vote: term {} is less than current term {}", arg.term,
                                   m_state.get_current_term());
                    return;
                }
                if (arg.term > m_state.get_current_term()) {
                    become_follower(arg.term);
                    return;
                }
                if (arg.vote_granted) {
                    votes_granted++;
                    if (votes_granted >= quorum) {
                        become_leader();
                        m_logger->debug("Received majority: {} votes (quorum: {})", votes_granted, quorum);
                    }
                }
            } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                m_logger->debug("Received RequestVote from candidate: {}", arg.candidate_id);
                if (arg.term <= m_state.get_current_term()) {
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = m_state.get_current_term();
                    arg.callback(response);
                    m_logger->debug("Denying vote to {}: candidate term ({}) is less than or equal to our term ({})",
                                    arg.candidate_id, arg.term, m_state.get_current_term());
                    return;
                }
                become_follower(arg.term);
                RequestVoteResponse response{};
                response.vote_granted = false;
                response.term = m_state.get_current_term();
                if (is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                    response.vote_granted = true;
                    m_state.set_voted_for(arg.candidate_id);
                    m_logger->debug("Granting vote to candidate {} because they have a higher term and up-to-date log",
                                    arg.candidate_id);
                } else {
                    m_logger->debug("Denying vote to candidate {} because candidate's log is not up-to-date",
                                    arg.candidate_id);
                }
                arg.callback(response);
            } else if constexpr (std::is_same_v<T, ClientRequestEvent>) {
                m_logger->critical("Have not implemented Client Request Handling");
                ClientRequestResponse response{};
                response.success = false;
                response.redirect = false;
                response.leader_id = m_leader_id;

                arg.callback(response);
            }
        }, event);

        if (should_exit) {
            return;
        }
    }
}

// returns true if the input log is as up-to-date or more up-to-date
bool raft::Node::is_log_more_up_to_date(unsigned int last_log_index, unsigned int last_log_term) const {
    if (last_log_term > m_state.get_last_log_term()) {
        return true;
    }
    if (last_log_term == m_state.get_last_log_term() && last_log_index >= m_state.get_last_log_index()) {
        return true;
    }
    return false;
}

void raft::Node::append_entries(unsigned int id, unsigned int lease_id) {
    std::string &address = m_cluster[id].address;
    AppendEntriesRPC append_entries_rpc;
    unsigned int last_index = m_state.get_last_log_index();

    append_entries_rpc.leader_id = m_id;
    append_entries_rpc.term = m_state.get_current_term();

    append_entries_rpc.prev_log_index = m_next_index[id] - 1;
    append_entries_rpc.prev_log_term = m_state.get_log_term(append_entries_rpc.prev_log_index);
    append_entries_rpc.entries = m_state.get_entries_till_end(m_next_index[id]);

    append_entries_rpc.commit_index = m_commit_index;

    m_client->append_entries(address, append_entries_rpc,
                             [this, address, id, last_index, lease_id](const AppendEntriesResponse &response) {
                                 if (response.error) {
                                     m_logger->error("AppendEntries failed for address {}", address);
                                     return;
                                 }
                                 AppendEntriesResponseEvent event{};
                                 event.term = static_cast<int>(response.term);
                                 event.success = response.success;
                                 event.id = id;
                                 event.last_index_added = last_index;
                                 event.lease_id = lease_id;
                                 m_event_queue.push(event);
                             });
}

void raft::Node::initialize_next_index() {
    unsigned int default_value = m_state.get_last_log_index() + 1;
    for (auto &[id, _]: m_cluster) {
        m_next_index[id] = default_value;
    }
}

void raft::Node::initialize_match_index() {
    for (auto &[id, _]: m_cluster) {
        m_match_index[id] = 0;
    }
}

void raft::Node::process_pending_requests() {
    ClientRequestResponse response{};
    response.success = true;
    response.redirect = false;
    response.leader_id = m_leader_id;

    auto new_end = std::ranges::remove_if(m_pending_requests,
                                          [this, &response](const PendingRequest &req) {
                                              if (req.waiting_commit_index <= m_commit_index) {
                                                  req.callback(response);
                                                  return true;
                                              }
                                              return false;
                                          }).begin();

    m_pending_requests.erase(new_end, m_pending_requests.end());
}

// this function checks the majority replicated
// index on each machine
void raft::Node::calculate_new_commit_index() {
    unsigned int quorum = calculate_quorum();
    std::vector<unsigned int> v;
    for (auto &[_, match]: m_match_index) {
        v.push_back(match);
    }
    std::ranges::sort(v);
    unsigned int candidate_commit_index = v[quorum - 1];
    if (m_state.get_log_term(candidate_commit_index) == m_state.get_current_term()) {
        update_commit_index(candidate_commit_index);
        m_logger->debug("Updating commit_index to {}", m_commit_index);
        process_pending_requests();
    }
}

void raft::Node::update_commit_index(unsigned int commit_index) {
    raft::FSMResponse response;
    // commit index can only increase
    // do not process indicies less than or
    // equal to ours
    if (m_commit_index >= commit_index) {
        return;
    }
    m_commit_index = commit_index;
    // applies commands from our last applied index
    // to the current commit index
    for (int i = m_last_applied_index + 1; i <= m_commit_index; i++) {
        std::optional<Log> log = m_state.read_log(i);
        if (log == std::nullopt) {
            m_logger->critical("Logs have been corrupted could not read log index: {}", i);
        }
        response = m_fsm->apply_command(log.value().entry);
        if (response.success == false) {
            m_logger->critical("Failed to apply command {} to State Machine; Reason: {}", log.value().entry,
                               response.data);
        }
    }
    m_last_applied_index = commit_index;
}

void raft::Node::clear_pending_requests() {
    ClientRequestResponse response{};
    response.success = false;
    response.redirect = false;
    response.leader_id = m_leader_id;

    // indicate the request failed
    for (auto &request: m_pending_requests) {
        request.callback(response);
    }

    m_pending_requests.clear();
}

void advance_lease_id(unsigned int &lease_id) {
    constexpr unsigned int MAX_LEASE_ID = 1000;
    lease_id = std::min(MAX_LEASE_ID, lease_id + 1);
}

void raft::Node::run_leader_loop() {
    initialize_match_index();
    initialize_next_index();
    bool should_exit = false;

    unsigned int lease_id = 1;
    bool valid_lease = true;
    // specify one to indicate our selves
    unsigned int current_lease_succesful_entries = 1;
    unsigned int quorum = calculate_quorum();

    while (m_server_state == ServerState::LEADER) {
        Event event = m_event_queue.pop();
        std::visit(
            [this, &should_exit, &valid_lease, &lease_id, &current_lease_succesful_entries, &quorum](auto &&arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, ElectionTimeoutEvent>) {
                    m_logger->warn("Election timeout received in Leader state");
                } else if constexpr (std::is_same_v<T, QuitEvent>) {
                    m_logger->debug("Quit signal received; stopping node");
                    stop();
                    should_exit = true;
                } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                    m_logger->debug("Heartbeat timeout received; sending AppendEntries to followers");
                    advance_lease_id(lease_id);

                    for (auto &[id, _]: m_cluster) {
                        if (id == m_id) continue;
                        append_entries(id, lease_id);
                    }
                    current_lease_succesful_entries = 1;

                    reset_heartbeat_timer();
                } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                    m_logger->debug("Received AppendEntries request from leader: {}", arg.leader_id);
                    if (arg.term < m_state.get_current_term()) {
                        AppendEntriesResponse response{};
                        response.term = m_state.get_current_term();
                        response.success = false;
                        arg.callback(response);
                        m_logger->debug(
                            "Denying AppendEntries request from {}: leader term ({}) is less than our term ({})",
                            arg.leader_id, arg.term, m_state.get_current_term());
                        return;
                    }

                    // if the append entries term is higher, demote to follower and accept the new leader
                    if (arg.term > m_state.get_current_term()) {
                        m_logger->debug("Accepting AppendEntries request and recognizing new leader");
                        become_follower(arg.term);
                        AppendEntriesResponse response{};
                        response.term = m_state.get_current_term();
                        response.success = true;
                        arg.callback(response);
                        return;
                    }
                    // This is the case where the term is equal; serious issue in the implementation.
                    m_logger->critical("Two leaders detected with the same term; denying AppendEntries request");
                    AppendEntriesResponse response{};
                    response.term = m_state.get_current_term();
                    response.success = false;
                    arg.callback(response);
                } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                    m_logger->debug("Received AppendEntriesResponse from: {}", arg.id);
                    if (arg.term > m_state.get_current_term()) {
                        become_follower(arg.term);
                        return;
                    }

                    if (arg.lease_id == lease_id) {
                        current_lease_succesful_entries++;
                        if (current_lease_succesful_entries >= quorum) {
                            m_logger->debug("Lease successfully renewed");
                            reset_lease_timer();
                            valid_lease = true;
                        }
                    }

                    if (!arg.success) {
                        m_next_index[arg.id] = std::max(1u, m_next_index[arg.id] - 1);
                    } else {
                        if (m_match_index[arg.id] != arg.last_index_added) {
                            m_logger->debug("{} Successfully replicated logs from index {} to {}", arg.id,
                                            m_next_index[arg.id] - 1,
                                            arg.last_index_added);
                            m_match_index[arg.id] = arg.last_index_added;
                            m_next_index[arg.id] = arg.last_index_added + 1;
                            calculate_new_commit_index();
                        }
                    }
                } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                    m_logger->warn("Received RequestVoteResponse in Leader state");
                    if (arg.term > m_state.get_current_term()) {
                        become_follower(arg.term);
                    }
                } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                    m_logger->warn("Received RequestVote from candidate: {}", arg.candidate_id);
                    if (arg.term <= m_state.get_current_term()) {
                        RequestVoteResponse response{};
                        response.vote_granted = false;
                        response.term = m_state.get_current_term();
                        arg.callback(response);
                        m_logger->warn(
                            "Denying vote to candidate {} because candidate term ({}) is less than our term ({})",
                            arg.candidate_id, arg.term, m_state.get_current_term());
                        return;
                    }
                    if (arg.term > m_state.get_current_term()) {
                        become_follower(arg.term);
                        RequestVoteResponse response{};
                        response.vote_granted = false;
                        response.term = m_state.get_current_term();
                        if (is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                            response.vote_granted = true;
                            m_state.set_voted_for(arg.candidate_id);
                            m_logger->debug(
                                "Granting vote to candidate {} and stepping down, since candidate has greater term",
                                arg.candidate_id);
                        } else {
                            m_logger->debug(
                                "Denying vote to candidate {}: candidate's term is greater but log is not up-to-date",
                                arg.candidate_id);
                        }
                        arg.callback(response);
                    }
                } else if constexpr (std::is_same_v<T, ClientRequestEvent>) {
                    if (m_state.append_log(arg.command) != SUCCESS) {
                        m_logger->warn("Failed to take clients request and append log");
                        ClientRequestResponse response{};
                        response.success = false;
                        response.redirect = false;
                        response.leader_id = m_leader_id;
                        arg.callback(response);
                        return;
                    }

                    // this will cause an append entry request to
                    // each of the followers
                    m_event_queue.push(HeartBeatEvent{});

                    PendingRequest request{};
                    request.waiting_commit_index = m_state.get_last_log_index();
                    request.callback = std::move(arg.callback);
                    m_pending_requests.push_back(request);
                } else if constexpr (std::is_same_v<T, LeaseTimeoutEvent>) {
                    m_logger->debug("Failed to get a successful append entries from majority thus lease is not valid");
                    valid_lease = false;

                    // triggers the acquisition of new
                    // lease
                    m_event_queue.push(HeartBeatEvent{});
                }
            }, event);

        if (should_exit) {
            return;
        }
    }
}

// calculates number of votes needed to win an election
int raft::Node::calculate_quorum() const {
    int num_voters = static_cast<int>(m_cluster.size());
    return num_voters / 2 + 1;
}

// Blocking call that causes the node to run
void raft::Node::run() {
    m_running = true;

    initialize();

    std::thread timer_thread([this] {
        m_io.run();
    });

    std::thread server_thread([this] {
        m_server.run(m_cluster[m_id].address);
    });
    m_logger->debug("Running gRPC server on: {}", m_cluster[m_id].address);

    while (m_running) {
        switch (m_server_state) {
            case ServerState::FOLLOWER:
                run_follower_loop();
                break;
            case ServerState::CANDIDATE:
                run_candidate_loop();
                break;
            case ServerState::LEADER:
                run_leader_loop();
                break;
        }
    }

    if (server_thread.joinable()) {
        server_thread.join();
    }

    if (timer_thread.joinable()) {
        timer_thread.join();
    }
}

std::string raft::server_state_to_str(ServerState state) {
    switch (state) {
        case ServerState::FOLLOWER:
            return "Follower";
        case ServerState::CANDIDATE:
            return "Candidate";
        case ServerState::LEADER:
            return "Leader";
        default:
            return "";
    }
}
