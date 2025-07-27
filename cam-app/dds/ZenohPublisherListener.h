// ZenohPublisherListener.h
#pragma once

#include "ZenohBase.h"
#include <atomic>

class ZenohPublisherListener : public ZenohBase {
protected:
    std::shared_ptr<zenoh::Publisher> publisher_;
    std::atomic<int> matched_{0};
    std::atomic<bool> first_connected_{false};

public:
    ZenohPublisherListener() = default;
    ~ZenohPublisherListener() override = default;

    bool init(const std::string& topic_name, 
              const std::string& partition) override {
        if (!ZenohBase::init(topic_name, partition)) {
            return false;
        }

        try {
            publisher_ = session_->declare_publisher(get_full_key());
            return true;
        } catch (const zenoh::ZenohException& e) {
            ERRORLOG("Failed to create publisher: {}", e.what());
            return false;
        }
    }

    template<typename T>
    bool write(const T& data) {
        try {
            publisher_->put(data);
            return true;
        } catch (const zenoh::ZenohException& e) {
            ERRORLOG("Failed to publish data: {}", e.what());
            return false;
        }
    }
};