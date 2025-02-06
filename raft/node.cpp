//
// Created by pknadimp on 1/18/25.

#include "node.h"

#include <thread>

#include "utils.h"

raft::Node::Node(unsigned int id, const ClusterMap &cluster, boost::asio::io_context &io,
                 std::unique_ptr<Client> client) : m_id(id),
                                                   m_cluster(cluster),
                                                   m_state("log_" + std::to_string(id) + ".txt"),
                                                   m_io(io),
                                                   m_election_timer(io),
                                                   m_strand(boost::asio::make_strand(io)),
                                                   m_work_guard(boost::asio::make_work_guard(io)),
                                                   m_client(std::move(client)) {
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

void raft::Node::reset_election_timer() {
    // Thread safe way to modify the election timers
    boost::asio::post(m_io, [this]() {
        m_election_timer.cancel();

        unsigned int random_time_ms = generate_random_number(ELECTION_TIMER_MIN_MS, ELECTION_TIMER_MAX_MS);
        m_election_timer.expires_after(boost::asio::chrono::milliseconds(random_time_ms));
        m_election_timer.async_wait([this](const boost::system::error_code &ec) {
            if (!ec) {
                m_event_queue.push(ElectionTimeout{});
            } else if (ec != boost::asio::error::operation_aborted) {
                std::cout << "Election timer error: " << ec.message() << std::endl;
            }
        });
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

void raft::Node::stop() {
    m_event_queue.push(QuitEvent{});
    shut_down_election_timer();
    m_work_guard.reset();
    m_io.stop();
}

void raft::Node::become_follower(unsigned int term) {
    std::cout << "Became Follower" << std::endl;
    m_server_state = ServerState::FOLLOWER;
    m_state.set_current_term(term);

    // Vote for no one
    m_state.set_voted_for(-1);
}

void raft::Node::become_leader() {
    std::cout << "Became Leader" << std::endl;
    m_server_state = ServerState::LEADER;
}

void raft::Node::run_follower_loop() {
    bool should_exit = false;
    while (m_server_state == ServerState::FOLLOWER) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeout>) {
                std::cout << "Election timeout: " << server_state_to_str(m_server_state) << " term: " << m_state.
                        get_current_term()
                        << std::endl;
                become_candidate();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                m_running = false;
                should_exit = true;
            } else if constexpr (std::is_same_v<T, RequestVoteResponse>) {
                std::cout << "WARNING: GOT VOTE AS FOLLOWER" << std::endl;
            }
        }, event);

        if (should_exit) {
            return;
        }
    }
}

void raft::Node::run_candidate_loop() {
    // set to one since we vote for our self
    int votes_granted = 1;
    int quorum = calculate_quorum();

    // Encapsulate logic into a start election function
    for (auto &pair: m_cluster) {
        // skip sending vote to ourselves
        if (pair.first == m_id) {
            continue;
        }
        std::string address = pair.second.ip + std::to_string(pair.second.port);
        m_client->request_vote(address, m_state.get_current_term(), m_id, m_state.get_last_log_index(),
                               m_state.get_last_log_term(), [this](int term, bool vote_granted) {
                                   m_event_queue.push(RequestVoteResponse{term, vote_granted});
                               });
    }

    bool should_exit = false;

    while (m_server_state == ServerState::CANDIDATE) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit, &votes_granted, &quorum](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeout>) {
                std::cout << "Election timeout: " << server_state_to_str(m_server_state) << " term: " << m_state.
                        get_current_term()
                        << std::endl;
                become_candidate();
                should_exit = true;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                m_running = false;
                should_exit = true;
            } else if constexpr (std::is_same_v<T, RequestVoteResponse>) {
                std::cout << "GOT VOTE" << std::endl;
                int term = arg.term;
                int vote_granted = arg.vote_granted;
                if (vote_granted) {
                    votes_granted++;
                    if (votes_granted >= quorum) {
                        // this will cause exit since candidate no longer leader
                        become_leader();
                    }
                }
            }
        }, event);

        if (should_exit) {
            return;
        }
    }
}


void raft::Node::run_leader_loop() {
    bool should_exit = false;

    while (m_server_state == ServerState::LEADER) {
        Event event = m_event_queue.pop();
        std::visit([this, &should_exit](auto &&arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, ElectionTimeout>) {
                std::cout << "WARNING: Got Election Timeout as a leader" << std::endl;
            } else if constexpr (std::is_same_v<T, QuitEvent>) {
                m_running = false;
                should_exit = true;
            } else if constexpr (std::is_same_v<T, RequestVoteResponse>) {
                std::cout << "WARNING: Got Request Vote Response as a leader" << std::endl;
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

// Blocking Call that causes the node to run
void raft::Node::run() {
    m_running = true;

    initialize();

    std::thread timer_thread([this] {
        m_io.run();
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
    std::cout << "Became Candidate" << std::endl;
    m_server_state = ServerState::CANDIDATE;

    m_state.increment_term();
    m_state.set_voted_for(m_id);
    reset_election_timer();
}
