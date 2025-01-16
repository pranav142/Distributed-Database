//
// Created by pknadimp on 1/15/25.
//

#ifndef WOL_H
#define WOL_H

namespace wol {
    enum class OpCode {
        WRITE,
    };

    enum ErrorCode {
        NO_ERROR,
        CANNOT_OPEN_FILE,
    };

    struct Log {
        OpCode OP;
        std::string key;
        std::string value;
    };

    class WOL {
    public:
        explicit WOL(const std::string &file_path);

        [[nodiscard]] ErrorCode write(const Log &log) const;

    private:
        std::string m_file_path;
    };

    std::string op_code_to_string(const OpCode& code);

    inline Log create_set_log(std::string key, std::string value) {
        return Log{OpCode::WRITE, std::move(key), std::move(value)};
    }
}

#endif //WOL_H
