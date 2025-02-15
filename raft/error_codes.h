//
// Created by pknadimp on 1/17/25.
//

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

namespace raft {
    enum ErrorCode {
        SUCCESS = 0,
        FAILED_TO_OPEN_FILE,
        LINE_OUT_OF_INDEX,
        FAILED_TO_PARSE_HEADER,
        INVALID_HEADER_INFO,
        FILE_NOT_FOUND,
        LOG_FORMAT_ISSUE,
        OUT_OF_DATE_LOG,
    };
};

#endif //ERROR_CODES_H
