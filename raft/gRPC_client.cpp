//
// Created by pknadimp on 1/27/25.
//

#include "gRPC_client.h"


void create_client() {
   grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
}
