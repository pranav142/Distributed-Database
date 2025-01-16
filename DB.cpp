//
// Created by pknadimp on 1/15/25.
//

#include "DB.h"
#include "wol.h"

namespace db {
    DB::DB(): m_wol(wol::WOL("example.txt")) {
    }

    ErrorCode DB::set(std::string key, std::string value) {
        if (int ret = m_wol.write(wol::create_set_log(key, value)); ret > 0) {
            return FAILED_TO_WRITE_TO_WOL;
        }

        m_kvs[std::move(key)] = std::move(value);
        return NO_ERROR;
    }

    std::optional<std::string> DB::get(const std::string &key) {
        if (!m_kvs.contains(key)) {
            return std::nullopt;
        }
        return m_kvs[key];
    }
}
