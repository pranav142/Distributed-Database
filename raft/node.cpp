//
// Created by pknadimp on 1/18/25.

#include "node.h"

#include <thread>

#include "gRPC_server.h"
#include "utils.h"

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io,
                 std::unique_ptr<Client> client) : m_id(id),
                                                   m_cluster(cluster),
                                                   m_state("log_" + std::to_string(id) + ".txt"),
                                                   m_io(io),
                                                   m_election_timer(io),
                                                   m_heartbeat_timer(io),
                                                   m_strand(boost::asio::make_strand(io)),
                                                   m_work_guard(boost::asio::make_work_guard(io)),
                                                   m_client(std::move(client)),
                                                   m_server(nullptr) {
}

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

void raft::Node::reset_election_timer() {
    // Thread safe way to modify the election timers
    boost::asio::post(m_io, [this]() {
        m_election_timer.cancel();

        unsigned int random_time_ms = generate_random_number(ELECTION_TIMER_MIN_MS, ELECTION_TIMER_MAX_MS);
        m_election_timer.expires_after(boost::asio::chrono::milliseconds(random_time_ms));
        m_election_timer.async_wait([this](const boost::system::error_code &ec) {
            if (!ec) {
                m_event_queue.push(ElectionTimeoutEvent{});
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cout << "Election timer error: " << ec.message() << std::endl;
            }
        });
    });
}

void raft::Node::reset_heartbeat_timer() {
    // Thread safe way to modify the election timers
    boost::asio::post(m_io, [this]() {
        m_heartbeat_timer.cancel();

        m_election_timer.expires_after(boost::asio::chrono::milliseconds(HEART_BEAT_INTERVAL_MS));
        m_election_timer.async_wait([this](const boost::system::error_code &ec) {
            if (!ec) {
                m_event_queue.push(HeartBeatEvent{});
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cout << "Heart beat timer error: " << ec.message() << std::endl;
            }
        });
    });
}


void raft::Node::shut_down_heartbeat_timer() {
    boost::asio::post(m_io, [this]() {
        m_heartbeat_timer.cancel();
    });
}

void raft::Node::initialize() {
    reset_election_timer();
}

void raft::Node::shut_down_election_timer() {
    boost::asio::post(m_io, [this]() {
        m_election_timer.cancel();
    });
}

void raft::Node::cancel() {
    m_event_queue.push(QuitEvent{});
}

void raft::Node::stop() {
    m_running = false;
    if (m_server) {
        m_server->Shutdown();
    }
    shut_down_election_timer();
    m_work_guard.reset();
    m_io.stop();
}

void raft::Node::become_follower(unsigned int term) {
    std::cout << m_id << " Became FOLLOWER" << std::endl;
    m_server_state = ServerState::FOLLOWER;

    reset_election_timer();
    shut_down_heartbeat_timer();
    m_state.set_current_term(term);
    m_state.set_vote_for_no_one();
}

void raft::Node::become_leader() {
    std::cout << m_id << " Became LEADER" << std::endl;
    m_server_state = ServerState::LEADER;

    // No more election timeouts when leader
    shut_down_election_timer();
    reset_heartbeat_timer();
    m_state.set_vote_for_no_one();
}

void raft::Node::run_follower_loop() {
    bool should_exit = false;
    while (m_server_state == ServerState::FOLLOWER && !should_exit) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeoutEvent>) {
                std::cout << m_id << " Election timeout: " << server_state_to_str(m_server_state) << " term: " <<
                        m_state.
                        get_current_term()
                        << std::endl;
                become_candidate();
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                stop();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                // the term of request is lower deny it
                if (arg.term < m_state.get_current_term()) {
                    AppendEntriesResponse response{};
                    response.success = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    return;
                }

                // if the term is higher adjust the term
                // and set bote for no one to indicate no vote has been
                // cast for this term
                if (arg.term > m_state.get_current_term()) {
                    m_state.set_current_term(arg.term);
                    m_state.set_vote_for_no_one();
                }

                reset_election_timer();
                AppendEntriesResponse response{};
                response.success = true;
                response.term = m_state.get_current_term();
                arg.callback(response);
            } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                std::cout << "WARNING: Got append entries response as a follower" << std::endl;
            } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                std::cout << "WARNING: Got heartbeat as a follower" << std::endl;
            } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                std::cout << m_id << " WARNING: GOT VOTE AS FOLLOWER" << std::endl;
            } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                std::cout << m_id << " Got request vote as follower" << std::endl;

                // If the request has a lower term
                // immediately deny the vote
                if (arg.term < m_state.get_current_term()) {
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    arg.callback(response);
                    should_exit = true;
                    return;
                }

                // If a valid RPC is sent
                // prevent unnecessary election
                reset_election_timer();

                // If the request has a higher term
                // update our term and indicate we haven't cast
                // a vote for new term
                if (arg.term > m_state.get_current_term()) {
                    m_state.set_current_term(arg.term);
                    m_state.set_voted_for(-1);
                }

                RequestVoteResponse response{};
                response.vote_granted = false;
                response.term = static_cast<int>(m_state.get_current_term());

                // If no vote has been cast
                // or already voted for candidate
                // only grant vote if the log is as much
                // or more up to date
                if ((m_state.has_voted_for_no_one() || m_state.get_voted_for() == arg.candidate_id) &&
                    is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                    response.vote_granted = true;
                    m_state.set_voted_for(arg.candidate_id);
                }

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

    m_client->request_vote(address, request_vote_rpc, [this](const RequestVoteResponse &response) {
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
                std::cout << m_id << " Election timeout: " << server_state_to_str(m_server_state) << " term: " <<
                        m_state.
                        get_current_term()
                        << std::endl;
                become_candidate();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                should_exit = true;
                stop();
            } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                if (arg.term < m_state.get_current_term()) {
                    AppendEntriesResponse response{};
                    response.term = m_state.get_current_term();
                    response.success = false;
                    arg.callback(response);
                    return;
                }

                // If a append entry is at least as large
                // as the candidates term become follower
                // and accept leader
                AppendEntriesResponse response{};
                become_follower(arg.term);
                response.term = m_state.get_current_term();
                response.success = true;
                arg.callback(response);
            } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                std::cout << "WARNING: Got append entries response as a candidate" << std::endl;
            } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                std::cout << "WARNING: Got heartbeat as a candidate" << std::endl;
            } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                std::cout << m_id << " GOT VOTE: Vote Granted: " << arg.vote_granted << " Term: " << arg.term <<
                        std::endl;

                // term from voter cannot be less than candidates term
                // unless this a response meant for a previous election
                if (arg.term < m_state.get_current_term()) {
                    std::cout << "WARNING: Received a outdated request vote response" << std::endl;
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
                    }
                }
            } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                std::cout << m_id << " Got request vote as candidate" << std::endl;

                // if the term is lower
                // immediately deny the vote
                if (arg.term < m_state.get_current_term()) {
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    arg.callback(response);
                    return;
                }

                if (arg.term > m_state.get_current_term()) {
                    become_follower(arg.term);
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    if (is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                        response.vote_granted = true;
                        m_state.set_voted_for(arg.candidate_id);
                    }
                    arg.callback(response);
                    return;
                }

                // this is the case when the term of request
                // is equal to the term of the candidate
                // in which case we deny the vote
                RequestVoteResponse request_vote_response{};
                request_vote_response.vote_granted = false;
                request_vote_response.term = static_cast<int>(m_state.get_current_term());
                arg.callback(request_vote_response);
            }
        }, event);

        if (should_exit) {
            return;
        }
    }
}

// returns true if the inputted log is as up to date or more up to date
bool raft::Node::is_log_more_up_to_date(unsigned int last_log_index, unsigned int last_log_term) const {
    if (last_log_term > m_state.get_last_log_term()) {
        return true;
    }

    if (last_log_term == m_state.get_last_log_term() && last_log_index >= m_state.get_last_log_index()) {
        return true;
    }

    return false;
}

void raft::Node::append_entries(const std::string &address) {
    AppendEntriesRPC append_entries_rpc;
    append_entries_rpc.commit_index = 0;
    append_entries_rpc.entries = "";
    append_entries_rpc.leader_id = m_id;
    append_entries_rpc.prev_log_index = 0;
    append_entries_rpc.prev_log_term = 0;
    append_entries_rpc.term = m_state.get_current_term();

    m_client->append_entries(address, append_entries_rpc, [this](const AppendEntriesResponse &response) {
        AppendEntriesResponseEvent event{};
        event.term = static_cast<int>(response.term);
        event.success = response.success;
        m_event_queue.push(event);
    });
}

void raft::Node::run_leader_loop() {
    bool should_exit = false;

    while (m_server_state == ServerState::LEADER) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeoutEvent>) {
                std::cout << m_id << " WARNING: Got Election Timeout as a leader" << std::endl;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                stop();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, HeartBeatEvent>) {
                for (auto &pair: m_cluster) {
                    if (pair.first == m_id) {
                        continue;
                    }
                    append_entries(pair.second.address);
                }
                reset_heartbeat_timer();
            } else if constexpr (std::is_same_v<T, AppendEntriesEvent>) {
                if (arg.term < m_state.get_current_term()) {
                    AppendEntriesResponse response{};
                    response.term = static_cast<int>(m_state.get_current_term());
                    response.success = false;
                    arg.callback(response);
                    return;
                }

                // if the append entries term is higher
                // demote to follower and accept the
                // new leader
                if (arg.term > m_state.get_current_term()) {
                    become_follower(arg.term);
                    AppendEntriesResponse response{};
                    response.term = static_cast<int>(m_state.get_current_term());
                    response.success = true;
                    arg.callback(response);
                    return;
                }

                // This is the case where the term is equal
                // if this occurred there is some serious issue
                // in the implementation
                std::cout << "ERROR: Received a append entries of equal term. Denying Request!" << std::endl;
                AppendEntriesResponse response{};
                response.term = static_cast<int>(m_state.get_current_term());
                response.success = false;
                arg.callback(response);
            } else if constexpr (std::is_same_v<T, AppendEntriesResponseEvent>) {
                if (arg.term > m_state.get_current_term()) {
                    become_follower(arg.term);
                    return;
                }
            } else if constexpr (std::is_same_v<T, RequestVoteResponseEvent>) {
                std::cout << m_id << " WARNING: Got Request Vote Response as a leader" << std::endl;
            } else if constexpr (std::is_same_v<T, RequestVoteEvent>) {
                std::cout << m_id << " Got request vote as a leader" << std::endl;

                if (arg.term < m_state.get_current_term()) {
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    arg.callback(response);
                    return;
                }

                if (arg.term > m_state.get_current_term()) {
                    become_follower(arg.term);
                    RequestVoteResponse response{};
                    response.vote_granted = false;
                    response.term = static_cast<int>(m_state.get_current_term());
                    if (is_log_more_up_to_date(arg.last_log_index, arg.last_log_term)) {
                        response.vote_granted = true;
                        m_state.set_voted_for(arg.candidate_id);
                    }
                    arg.callback(response);
                    return;
                }

                // if the term is equal
                // reject the request to vote
                RequestVoteResponse response{};
                response.vote_granted = false;
                response.term = static_cast<int>(m_state.get_current_term());
                arg.callback(response);
            }
        }, event);

        if (should_exit) {
            return;
        }
    }
}

// calculates number of votes needed to win a election
int raft::Node::calculate_quorum() const {
    int num_voters = static_cast<int>(m_cluster.size());
    return num_voters / 2 + 1;
}

void raft::Node::run_server(const std::string &address) {
    m_service = std::make_unique<RaftSeverImpl>(m_event_queue);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(m_service.get());
    m_server = builder.BuildAndStart();
    std::cout << "Server listening on " << address << std::endl;
    m_server->Wait();
}

// Blocking Call that causes the node to run
void raft::Node::run() {
    m_running = true;

    initialize();

    std::thread timer_thread([this] {
        m_io.run();
    });

    std::thread server_thread([this] {
        run_server(m_cluster[m_id].address);
    });

    while (m_running) {
        switch (m_server_state) {
            case ServerState::FOLLOWER: {
                run_follower_loop();
                break;
            }
            case ServerState::CANDIDATE: {
                run_candidate_loop();
                break;
            }
            case ServerState::LEADER: {
                run_leader_loop();
                break;
            }
        }
    }

    server_thread.join();
    timer_thread.join();
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

void raft::Node::become_candidate() {
    std::cout << m_id << " Became CANDIDATE" << std::endl;
    m_server_state = ServerState::CANDIDATE;

    m_state.increment_term();
    m_state.set_voted_for(static_cast<int>(m_id));
    reset_election_timer();
}
