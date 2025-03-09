//
// Created by pknadimp on 2/19/25.
//

#include <gtest/gtest.h>
#include "logging.h"
#include "lb.h"
#include "key_value/command.h"
#include "cluster.h"
#include "gRPC_client.h"
#include "node.h"
#include "raft/gRPC_client.h"
#include "key_value/db.h"
#include "curl/curl.h"

size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

std::string send_post_request(const std::string& url, const loadbalancer::LBClientRequest& req) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize CURL" << std::endl;
        return "";
    }

    std::string serialized_request = utils::serialized_data_to_string(req.request);

    char* key_encoded = curl_easy_escape(curl, req.key.c_str(), req.key.length());
    char* request_encoded = curl_easy_escape(curl, serialized_request.c_str(), serialized_request.length());

    if (!key_encoded || !request_encoded) {
        std::cerr << "Failed to URL-encode parameters" << std::endl;
        curl_easy_cleanup(curl);
        return "";
    }

    std::string post_fields = "key=";
    post_fields += key_encoded;
    post_fields += "&request=";
    post_fields += request_encoded;

    curl_free(key_encoded);
    curl_free(request_encoded);

    std::string response_data;

    // Use the globally defined write_callback function, not the lambda
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_fields.c_str());

    // Use the global write_callback function
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return response_data;
}

std::vector<std::unique_ptr<raft::Node> > create_nodes(const utils::ClusterMap &cluster_map) {
    std::vector<std::unique_ptr<raft::Node> > nodes;

    for (auto &[id, node_info]: cluster_map) {
        auto client = std::make_unique<raft::gRPCClient>();
        auto fsm = std::make_shared<kv::DB>();
        auto node = std::make_unique<raft::Node>(id, cluster_map, std::move(client), fsm);
        nodes.push_back(std::move(node));
    }

    return nodes;
}

utils::ClusterMap cluster_map_1{
    {0, utils::NodeInfo{"127.0.0.1:6969"}},
    {1, utils::NodeInfo{"127.0.0.1:7070"}},
    {2, utils::NodeInfo{"127.0.0.1:4206"}},
};

utils::ClusterMap cluster_map_2{
    {3, utils::NodeInfo{"127.0.0.1:6970"}},
    {4, utils::NodeInfo{"127.0.0.1:7071"}},
    {5, utils::NodeInfo{"127.0.0.1:4207"}},
};

utils::ClusterMap cluster_map_3{
    {6, utils::NodeInfo{"127.0.0.1:6981"}},
    {7, utils::NodeInfo{"127.0.0.1:7171"}},
    {8, utils::NodeInfo{"127.0.0.1:4217"}},
};


utils::Clusters clusters{
    {"Cluster1", cluster_map_1},
    {"Cluster2", cluster_map_2},
    {"Cluster3", cluster_map_3}
};

TEST(LoadBalancerTests, HandlesRequestRouting) {
    utils::initialize_global_logging();


    loadbalancer::LoadBalancer lb(clusters, 30);

    auto cluster_1 = create_nodes(cluster_map_1);
    auto cluster_2 = create_nodes(cluster_map_2);
    auto cluster_3 = create_nodes(cluster_map_3);

    std::vector<std::thread> node_threads;

    auto run_cluster = [&](auto &cluster) {
        for (auto &node: cluster) {
            node_threads.emplace_back([&node]() {
                node->run();
            });
        }
    };

    run_cluster(cluster_1);
    run_cluster(cluster_2);
    run_cluster(cluster_3);

    std::thread request_thread([&]() {
        // correctly sets the values for keys
        std::this_thread::sleep_for(std::chrono::milliseconds(raft::ELECTION_TIMER_MAX_MS * 10));
        for (int i = 0; i < 100; i++) {
            kv::Request request;
            request.key = "key_" + std::to_string(i);
            request.value = "value_" + std::to_string(i);
            request.type = kv::RequestType::SET;


            loadbalancer::LBClientRequest client_request;
            utils::SerializedData serialized_request = kv::serialize_request(request);
            client_request.key = request.key;
            client_request.request = serialized_request;

            loadbalancer::LBClientResponse response = lb.process_request(client_request);
            GTEST_ASSERT_TRUE(response.error_code == loadbalancer::LBClientResponse::ErrorCode::SUCCESS);
        }

        // correctly handles fetching keys
        // that have been set
        for (int i = 0; i < 100; i++) {
            kv::Request request;
            request.key = "key_" + std::to_string(i);
            request.type = kv::RequestType::GET;

            loadbalancer::LBClientRequest client_request;
            utils::SerializedData serialized_request = kv::serialize_request(request);
            client_request.key = request.key;
            client_request.request = serialized_request;

            loadbalancer::LBClientResponse response = lb.process_request(client_request);
            GTEST_ASSERT_TRUE(response.error_code == loadbalancer::LBClientResponse::ErrorCode::SUCCESS);

            std::optional<kv::Response> deserialized_response = kv::deserialize_response(response.response);
            GTEST_ASSERT_TRUE(deserialized_response.has_value());
            GTEST_ASSERT_TRUE(deserialized_response->data == "value_" + std::to_string(i));
        }

        // correctly handles fetching keys
        // that have not been set
        for (int i = 100; i < 150; i++) {
            kv::Request request;
            request.key = "key_" + std::to_string(i);
            request.type = kv::RequestType::GET;

            loadbalancer::LBClientRequest client_request;
            utils::SerializedData serialized_request = kv::serialize_request(request);
            client_request.key = request.key;
            client_request.request = serialized_request;

            loadbalancer::LBClientResponse response = lb.process_request(client_request);
            GTEST_ASSERT_TRUE(response.error_code == loadbalancer::LBClientResponse::ErrorCode::SUCCESS);

            std::optional<kv::Response> deserialized_response = kv::deserialize_response(response.response);
            GTEST_ASSERT_TRUE(deserialized_response.has_value());
            GTEST_ASSERT_TRUE(deserialized_response->data == "null");
        }

        // deletes keys
        for (int i = 0; i < 100; i++) {
            kv::Request request;
            request.key = "key_" + std::to_string(i);
            request.value = "value_" + std::to_string(i);
            request.type = kv::RequestType::DELETE;


            loadbalancer::LBClientRequest client_request;
            utils::SerializedData serialized_request = kv::serialize_request(request);
            client_request.key = request.key;
            client_request.request = serialized_request;

            loadbalancer::LBClientResponse response = lb.process_request(client_request);
            GTEST_ASSERT_TRUE(response.error_code == loadbalancer::LBClientResponse::ErrorCode::SUCCESS);
        }

        // ensures keys are deleted
        for (int i = 0; i < 100; i++) {
            kv::Request request;
            request.key = "key_" + std::to_string(i);
            request.type = kv::RequestType::GET;

            loadbalancer::LBClientRequest client_request;
            utils::SerializedData serialized_request = kv::serialize_request(request);
            client_request.key = request.key;
            client_request.request = serialized_request;

            loadbalancer::LBClientResponse response = lb.process_request(client_request);
            GTEST_ASSERT_TRUE(response.error_code == loadbalancer::LBClientResponse::ErrorCode::SUCCESS);

            std::optional<kv::Response> deserialized_response = kv::deserialize_response(response.response);
            GTEST_ASSERT_TRUE(deserialized_response.has_value());
            GTEST_ASSERT_TRUE(deserialized_response->data == "null");
        }

        std::array clusters_{std::ref(cluster_1), std::ref(cluster_2), std::ref(cluster_3)};
        for (auto &cluster: clusters_) {
            for (auto &node: cluster.get()) {
                node->cancel();
            }
        }
    });


    request_thread.join();
    for (auto &t: node_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

TEST(LoadBalancerTests, IntegrateLBWithHttp) {
    constexpr int PORT = 8080;
    utils::initialize_global_logging();

    loadbalancer::LoadBalancer lb(clusters, 30);

    auto cluster_1 = create_nodes(cluster_map_1);
    auto cluster_2 = create_nodes(cluster_map_2);
    auto cluster_3 = create_nodes(cluster_map_3);

    std::vector<std::thread> node_threads;

    auto run_cluster = [&](auto &cluster) {
        for (auto &node: cluster) {
            node_threads.emplace_back([&node]() {
                node->run();
            });
        }
    };

    run_cluster(cluster_1);
    run_cluster(cluster_2);
    run_cluster(cluster_3);

    std::thread http_request_thread([&]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        for (int i = 0; i < 100; i++) {
            kv::Request request;
            request.key = "http_key_" + std::to_string(i);
            request.value = "value_" + std::to_string(i);
            request.type = kv::RequestType::SET;

            // HTTP Request (Body):
            //      key: <string>
            //      data(serialized): <string>
            loadbalancer::LBClientRequest client_request;
            client_request.key = request.key;
            client_request.request = kv::serialize_request(request);

            std::string response = send_post_request("http://localhost:" + std::to_string(PORT) + "/request", client_request);
        }

        std::array clusters_{std::ref(cluster_1), std::ref(cluster_2), std::ref(cluster_3)};
        for (auto &cluster: clusters_) {
            for (auto &node: cluster.get()) {
                node->cancel();
            }
        }

        lb.shutdown();
    });

    lb.run(PORT);

    for (auto &t: node_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    if (http_request_thread.joinable()) {
        http_request_thread.join();
    }
}
