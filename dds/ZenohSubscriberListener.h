// ZenohSubscriberListener.h
#pragma once

#include "ZenohBase.h"
#include <atomic>

class ZenohSubscriberListener : public ZenohBase {
protected:
    std::shared_ptr<zenoh::Subscriber> subscriber_;
    std::atomic<int> matched_{0};
    std::atomic<uint32_t> samples_{0};
    bool need_monitor_{false};

public:
    ZenohSubscriberListener() = default;
    ~ZenohSubscriberListener() override = default;

    bool init(const std::string& topic_name, 
              const std::string& partition) override {
        if (!ZenohBase::init(topic_name, partition)) {
            return false;
        }

        try {
            subscriber_ = session_->declare_subscriber(
                get_full_key(),
                [this](const zenoh::Sample& sample) {
                    this->on_data_available(sample);
                }
            );
            return true;
        } catch (const zenoh::ZenohException& e) {
            ERRORLOG("Failed to create subscriber: {}", e.what());
            return false;
        }
    }

    virtual void on_data_available(const zenoh::Sample& sample) = 0;
};