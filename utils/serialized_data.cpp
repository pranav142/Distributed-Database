//
// Created by pknadimp on 2/27/25.
//

#include "serialized_data.h"

std::string utils::serialized_data_to_string(const SerializedData& data) {
    return {data.begin(), data.end()} ;
}

utils::SerializedData utils::serialized_data_from_string(const std::string& data) {
    return {data.begin(), data.end()};
}
