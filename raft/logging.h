//
// Created by pknadimp on 2/12/25.
//

// This file is responsible for logging
// information during the raft nodes operation
// it should not be confused with the logs
// that raft stores for replication

#ifndef LOGGING_H
#define LOGGING_H


namespace raft {
    void initialize_global_logging();
}

#endif //LOGGING_H
