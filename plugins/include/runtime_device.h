#ifndef RUNTIME_DEVICE_H
#define RUNTIME_DEVICE_H

#include <string>
#include <vector>

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

    Device(char* name, char* uuid, int busID): busID(busID)
    {
        if (name)
            this->name = name;

        if (uuid)
            this->uuid = uuid;
    }

    ~Device() = default;

    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = default;
    Device& operator=(Device&&) = default;

private:

    std::string name;
    std::string uuid;
    int  busID = 0;
    
};

    typedef std::vector<Device> Devices;

#endif // RUNTIME_DEVICE_H