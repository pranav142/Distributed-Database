//
// Created by pknadimp on 1/17/25.
//

#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "error_codes.h"
#include <vector>

namespace raft {
    ErrorCode replace_line(const std::string &file_path, unsigned int line_num, const std::string &value);

    std::vector<std::string> read_file(const std::string &file_path);

    void clear_file(const std::string &file_path);

    std::string get_last_line(const std::string &file_path);

    ErrorCode append_to_file(const std::string &file_path, const std::string &value);
}


#endif //UTILS_H
