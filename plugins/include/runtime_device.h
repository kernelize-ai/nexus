#ifndef RUNTIME_DEVICE_H
#define RUNTIME_DEVICE_H

#include <string>
#include <vector>
#include <memory>

enum class DeviceType {
    GPU = 0,
    ACCELERATOR
};

enum class DeviceState {
    UNKNOWN = 0,
    PRESENT,
    ACTIVE,
    ERROR
};

struct DeviceID {
    uint16_t vendor_id = 0;
    uint16_t device_id = 0;
};

class Device {
public:

    Device() {};
    ~Device() {};

    // No copy, allow move
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

    // Basic device info
    bool probe();
    bool is_present() const { return state_ == DeviceState::PRESENT || state_ == DeviceState::ACTIVE; }
    DeviceType get_device_type() const { return device_type_; }
    DeviceState get_state() const { return state_; }
    const DeviceID& get_device_id() const { return device_id_; }
    const std::string& get_sysfs_path() const { return sysfs_path_; }

private:

    DeviceID device_id_;
    DeviceType device_type_;
    DeviceState state_ = DeviceState::UNKNOWN;
    std::string sysfs_path_;
    
    bool parse_device_info();
    DeviceType determine_device_type() const;

};

    typedef std::vector<Device> Devices;

#endif // RUNTIME_DEVICE_H