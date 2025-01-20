#include <iostream>

#include "libs/httplib/httplib.h"
#include "raft/node.h"

int main() {
    std::cout << "Hello, World!" << std::endl;
    //httplib::Server svr;
    //db::DB db;

    //svr.Get("/hi", [](const httplib::Request &req, httplib::Response &res) {
    //  res.set_content("Hello World!", "text/plain");
    //});

    //svr.Post("/set", [&db](const httplib::Request &req, httplib::Response &res) {
    //    if (!req.has_param("key") || !req.has_param("value")) {
    //        res.set_content("Key or Value not specified", "text/plain");
    //        return;
    //    }
    //    if (int ret = db.set(req.get_param_value("key"), req.get_param_value("value")); ret != 0) {
    //        res.set_content("Failed to set key value", "text/plain");
    //        return;
    //    }
    //    res.set_content("Success!", "text/plain");
    //});

    //svr.Get("/get", [&db](const httplib::Request &req, httplib::Response &res) {
    //    if (!req.has_param("key")) {
    //        res.set_content("Key not specified", "text/plain");
    //        return;
    //    }
    //    auto val = db.get(req.get_param_value("key"));
    //    if (val == std::nullopt) {
    //        res.set_content("Could not fetch data", "text/plain");
    //        return;
    //    }
    //    res.set_content(val.value_or("NULL"), "text/plain");
    //});

    //svr.listen("localhost", 8080);
    //return 0;
}
