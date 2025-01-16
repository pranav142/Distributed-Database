//
// Created by pknadimp on 1/15/25.
//

#ifndef DB_H
#define DB_H

#include <unordered_map>
#include <string>
#include <optional>
#include "wol.h"

namespace db {
    enum ErrorCode {
        NO_ERROR,
        FAILED_TO_WRITE_TO_WOL,
    };

    class DB {
    public:
        DB();

        ErrorCode set(std::string key, std::string value);

        std::optional<std::string> get(const std::string &key);

    private:
        std::unordered_map<std::string, std::string> m_kvs;
        wol::WOL m_wol;
    };

}
#endif //DB_H
