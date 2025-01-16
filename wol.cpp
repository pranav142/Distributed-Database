//
// Created by pknadimp on 1/15/25.
//

#include "wol.h"
#include <filesystem>

namespace wol {
    WOL::WOL(const std::string &file_path) {
        m_file_path = file_path;
    }

    ErrorCode WOL::write(const Log &log) const {
        std::ofstream file(m_file_path, std::ios::app);
        if (!file.is_open()) {
            std::cerr << "Failed to open file " << m_file_path << std::endl;
            return CANNOT_OPEN_FILE;
        }
        file << op_code_to_string(log.OP) << "," <<  log.key << "," << log.value << std::endl;
        file.close();
        return NO_ERROR;
    }

    std::string op_code_to_string(const OpCode &code) {
        switch (code) {
            case OpCode::WRITE:
                return "write";
            default:
                return "invalid_op";
        }
    }
}
