//
// Created by pknadimp on 1/20/25.
//

#include <gtest/gtest.h>
#include "event_queue.h"

TEST(EventQueueTests, HandlesValidInputs) {
    utils::EventQueue<int> event_queue;

    event_queue.push(2);
    int return_val = event_queue.pop();
    ASSERT_EQ(return_val, 2);
}

TEST(EventQueueTests, HandlesThreadSafety) {
    utils::EventQueue<int> event_queue;

    // put this thread to sleep to test whether pop will sleep until data is available
    std::thread t([&event_queue]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        event_queue.push(2);
        event_queue.push(3);
    });

    int return_val = event_queue.pop();
    ASSERT_EQ(return_val, 2);

    return_val = event_queue.pop();
    ASSERT_EQ(return_val, 3);

    t.join();
}

