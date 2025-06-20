#ifndef RUNTIME_DEVICE_H
#define RUNTIME_DEVICE_H

#include <string>
#include <vector>
#include <regex>

#include <runtime_library.h>
#include <runtime_schedule.h>

#define CHECK_CU(call) \
    do { \
        CUresult err = call; \
        if (err != CUDA_SUCCESS) { \
            const char* errorStr; \
            cuGetErrorString(err, &errorStr); \
            std::cerr << "CUDA Error: " << errorStr << std::endl; \
            exit(1); \
        } \
    } while(0)

#define CHECK_CUDA(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            std::cerr << "CUDA Runtime Error: " << cudaGetErrorString(err) << std::endl; \
            exit(1); \
        } \
    } while(0)

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

        cuDeviceGet(&cudaDeviceRef, 0);

        cuCtxCreate(&context, 0, cudaDeviceRef);
    }

    ~Device() = default;

    nxs_int createLibrary(void *library_data, nxs_uint data_size) {
      libraries.emplace_back(library_data, data_size);
      RuntimeLibrary &l = libraries[libraries.size()-1];
      CHECK_CU(cuModuleLoadData(&l.module, library_data));
        
      std::string ptx_content(static_cast<const char*>(library_data), data_size);
      std::regex kernel_regex(R"(\.visible\s+\.entry\s+(\w+)\s*\()");
      std::smatch matches;
      if (std::regex_search(ptx_content, matches, kernel_regex))
        l.kernel.name = matches[1].str();
      else
        return -1;

      CHECK_CU(cuModuleGetFunction(&l.kernel.kernel, l.module, l.kernel.name.c_str()));
      return libraries.size() - 1;
    }

    nxs_int createSchedule() {
        schedules.emplace_back();
        return schedules.size() - 1;
    }

    nxs_status runSchedule(nxs_int schedule_id, nxs_bool blocking) {

        if (schedule_id >= schedules.size())
            return NXS_InvalidSchedule;

        for (auto command : schedules[schedule_id].commands) {

          int n = 1024;

          std::vector<void*> args;
          for (auto& argObj : command.buffers)
            args.push_back(reinterpret_cast<void*>(&argObj.ptr));

          args.push_back(reinterpret_cast<void*>(&n));

          CUresult cu_result = cuLaunchKernel(command.library.kernel.kernel,
                                    command.gridSize, 1, 1,
                                    command.blockSize, 1, 1,
                                    0,
                                    nullptr,
                                    args.data(),
                                    nullptr);
          if (cu_result != CUDA_SUCCESS)
              return NXS_InvalidKernel;

          CHECK_CUDA(cudaDeviceSynchronize());
        }

      return NXS_Success;
    }

    std::string name;
    std::string uuid;
    int  busID = 0;

    Libraries libraries;
    Schedules schedules;

    CUcontext context;
    CUdevice cudaDeviceRef;

};

    typedef std::vector<Device> Devices;

#endif // RUNTIME_DEVICE_H