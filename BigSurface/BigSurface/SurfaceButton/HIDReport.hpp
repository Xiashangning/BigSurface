//
//  HIDReport.hpp
//  SurfaceButtonDriver
//
//  Copyright © 2019 Le Bao Hiep. All rights reserved.
//

class __attribute__((packed)) keys final {
public:
    keys(void) : keys_{} {}

    const UInt8(&get_raw_value(void) const)[32] {
        return keys_;
    }

    bool empty(void) const {
        for (const auto& k : keys_) {
            if (k != 0) {
                return false;
            }
        }
        return true;
    }

    void clear(void) {
        memset(keys_, 0, sizeof(keys_));
    }

    void insert(UInt8 key) {
        if (!exists(key)) {
            for (auto&& k : keys_) {
                if (k == 0) {
                    k = key;
                    return;
                }
            }
        }
    }

    void erase(UInt8 key) {
        for (auto&& k : keys_) {
            if (k == key) {
                k = 0;
            }
        }
    }

    bool exists(UInt8 key) const {
        for (const auto& k : keys_) {
            if (k == key) {
                return true;
            }
        }
        return false;
    }

    size_t count(void) const {
        size_t result = 0;
        for (const auto& k : keys_) {
            if (k) {
                ++result;
            }
        }
        return result;
    }

    bool operator==(const keys& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
    bool operator!=(const keys& other) const { return !(*this == other); }

private:
    UInt8 keys_[32];
};

class __attribute__((packed)) consumer_input final {
public:
    consumer_input(void) : report_id_(2) {}
    bool operator==(const consumer_input& other) const { return (memcmp(this, &other, sizeof(*this)) == 0); }
    bool operator!=(const consumer_input& other) const { return !(*this == other); }

private:
    UInt8 report_id_ __attribute__((unused));

public:
    keys keys;
};
