# Distributed Database System

A scalable, fault-tolerant distributed key-value database using the Raft consensus protocol and consistent hashing for load balancing across clusters.

## Architecture Overview

This system is built as a distributed database with the following key components:

### Core Components

1. **Load Balancer**
   - Routes client requests to appropriate clusters using consistent hashing
   - Maintains leader cache to optimize request routing
   - Provides HTTP API for client interactions

2. **Raft Consensus Protocol**
   - Ensures data consistency across replicated nodes
   - Implements leader election, log replication, and safety
   - Manages cluster state and leadership changes

3. **Key-Value Store (FSM)**
   - Simple key-value database implementation
   - Supports GET, SET, and DELETE operations
   - Implements the Finite State Machine interface for Raft

### System Design

```
+------------------+
|                  |
|   HTTP Clients   |
|                  |
+--------+---------+
         |
         v
+--------+---------+
|                  |
|  Load Balancer   |  <-- Consistent hashing for routing
|                  |
+--------+---------+
         |
         v
+--------+--------+      +--------+--------+      +--------+--------+
|                 |      |                 |      |                 |
| Raft Cluster 1  | <--> | Raft Cluster 2  | <--> | Raft Cluster 3  |
|                 |      |                 |      |                 |
+-----------------+      +-----------------+      +-----------------+
```

- **Multi-cluster Architecture**: Data is distributed across multiple Raft clusters
- **Consistent Hashing**: Provides balanced distribution of keys and minimizes redistribution during scaling
- **Raft Consensus**: Each cluster maintains data consistency using the Raft protocol
- **Leader Cache**: Optimization to reduce redirects by caching cluster leaders

## Getting Started

### Prerequisites

- CMake 3.29 or higher
- C++ compiler with C++20 support (Clang recommended)
- Protocol Buffers
- gRPC
- Boost libraries
- TBB (Threading Building Blocks)
- Crow (for HTTP server)
- CURL (for HTTP client)
- spdlog (for logging)

### Building the Project

```bash
# Clone the repository
git clone https://github.com/yourusername/distributed-db.git
cd distributed-db

# Create build directory
mkdir build && cd build

# Configure and build
cmake ..
make
```

### Running Tests

```bash
# Run all tests
ctest

# Run specific test suites
./GTest_raft_run
./GTest_FSM_run
./GTest_util_run
./GTest_lb_run
```

## Usage

### Starting the System

The main program provided is a complete example that demonstrates the nodes and load balancer working together. Simply run:

```bash
./main
```

This will spin up:
- 9 nodes across 3 clusters (similar to the test configuration)
- A load balancer on port 8080
- All components configured to work together out of the box

The example automatically handles:
- Node initialization and leader election
- Load balancer setup with consistent hashing
- Graceful shutdown when receiving Ctrl+C

### Making Requests

The database supports standard key-value operations through HTTP:

```bash
# SET operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=SET|mykey|myvalue"

# GET operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=GET|mykey|"

# DELETE operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=DELETE|mykey|"
```

These commands will work with the example configuration provided by the main program.

### Example Responses

When using the system, here are examples of responses you'll receive:

```
# SET operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=SET|mykey|myvalue"
{"response":"","error_code":"SUCCESS"}

# GET operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=GET|mykey|"
{"response":"1|myvalue","error_code":"SUCCESS"}

# DELETE operation
curl -X POST http://localhost:8080/request \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "key=mykey&request=DELETE|mykey|"
{"response":"","error_code":"SUCCESS"}
```

The response format is JSON with two fields:
- `error_code`: Indicates the result of the operation ("SUCCESS" or an error message)
- `response`: Contains the returned data (empty for SET/DELETE, contains value for GET)

## Technical Details

### Raft Implementation

The Raft consensus algorithm implementation includes:

- Leader election with randomized timeouts
- Log replication with term checking
- Safety guarantees through log matching
- Membership changes (adding/removing nodes)
- Client request handling and forwarding to leaders

### Load Balancer

The load balancer provides:

- Consistent hashing for request distribution
- Leader caching to avoid redirects
- HTTP API for client interactions
- Request retries for fault tolerance

### Finite State Machine (FSM)

The key-value store implements the FSM interface for Raft:

- Commands that modify state (SET, DELETE)
- Queries that read state (GET)
- Serialization/deserialization of requests and responses

## Acknowledgments

- The Raft consensus algorithm by Diego Ongaro and John Ousterhout
- Consistent hashing concept by David Karger et al.