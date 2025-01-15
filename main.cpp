#include <iostream>
#include <unordered_map>
#include "libs/httplib/httplib.h"


int main() {
    std::cout << "Hello, World!" << std::endl;

    httplib::Server svr;
    std::unordered_map<std::string, std::string> kvs;

    svr.Get("/hi", [](const httplib::Request &req, httplib::Response &res) {
      res.set_content("Hello World!", "text/plain");
    });

    svr.Post("/set", [&kvs](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("key") || !req.has_param("value")) {
            res.set_content("Key or Value not specified", "text/plain");
            return;
        }
        std::string key = req.get_param_value("key");
        std::string value = req.get_param_value("value");
        std::cout << "Key: " << key << " Value: " << value << std::endl;
        kvs[key] = value;
        res.set_content("Success!", "text/plain");
    });

    svr.Get("/get", [&kvs](const httplib::Request &req, httplib::Response &res) {
        if (!req.has_param("key")) {
            res.set_content("Key not specified", "text/plain");
            return;
        }
        std::string key = req.get_param_value("key");
        if (kvs.find(key) == kvs.end()) {
            res.set_content("Key not found", "text/plain");
            return;
        }
        res.set_content(kvs[key], "text/plain");
    });

    svr.listen("localhost", 8080);
    return 0;
}
