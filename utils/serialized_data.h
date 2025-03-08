//
// Created by pknadimp on 2/27/25.
//

#ifndef SERIALIZED_DATA_H
#define SERIALIZED_DATA_H

#include <cstdint>
#include <string>
#include <vector>

namespace util {
    typedef std::vector<uint8_t> SerializedData;

    std::string serialized_data_to_string(const SerializedData& data);

    SerializedData serialized_data_from_string(const std::string& data);
}


#endif //SERIALIZED_DATA_H
