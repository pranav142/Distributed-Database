//
// Created by pknadimp on 1/17/25.
//

#include "utils.h"
#include <fstream>
#include <vector>
#include <random>

// Value should have a new line character at the end
// the line number starts at 1
raft::ErrorCode raft::replace_line(const std::string &file_path, unsigned int line_num, const std::string &value) {
    std::ofstream write_file;
    std::vector<std::string> lines = read_file(file_path);
    line_num--;

    if (line_num > lines.size()) {
        return LINE_OUT_OF_INDEX;
    }

    write_file.open(file_path);
    if (write_file.fail()) {
        return FAILED_TO_OPEN_FILE;
    }

    for (int i = 0; i < lines.size(); i++) {
        if (i == line_num) {
            write_file << value;
        } else {
            write_file << lines[i] << '\n';
        }
    }

    // If the file is empty it is allowed write to the first line
    if (lines.empty() && line_num == 0) {
        write_file << value;
    }

    write_file.close();
    return SUCCESS;
}

std::vector<std::string> raft::read_file(const std::string &file_path) {
    std::ifstream file(file_path);
    if (file.fail()) {
        return {};
    }

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    file.close();
    return lines;
}

void raft::clear_file(const std::string &file_path) {
    std::ofstream file;
    file.open(file_path, std::ios::trunc);
    file.close();
}

std::string raft::get_last_line(const std::string &file_path) {
    std::ifstream file(file_path);
    std::string line, last_line;

    while (std::getline(file, line)) {
        last_line = line;
    }

    return last_line;
}

raft::ErrorCode raft::append_to_file(const std::string &file_path, const std::string &value) {
    std::ofstream log_file;
    log_file.open(file_path, std::ios::app);
    if (log_file.fail()) {
        return FAILED_TO_OPEN_FILE;
    }

    std::string content = value;
    if (!content.empty() && content.back() != '\n') {
        content += '\n'; // Add a newline if it's missing
    }
    log_file << content;
    log_file.close();
    return SUCCESS;
}

int raft::generate_random_number(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max); // Generate integers from 1 to 6 (inclusive)
    return dist(gen);
}
