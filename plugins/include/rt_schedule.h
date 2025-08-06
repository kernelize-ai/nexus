#ifndef RT_SCHEDULE_H
#define RT_SCHEDULE_H

#include <rt_command.h>

namespace nxs {
namespace rt {

template <typename Tcommand, typename Tstream>
class Schedule {
  typedef std::vector<Tcommand *> Commands;

 protected:
  nxs_int device_id;
  nxs_uint settings;
  Commands commands;

 public:
  Schedule(nxs_int dev_id = -1, nxs_uint settings = 0)
      : device_id(dev_id), settings(settings) {
    commands.reserve(8);
  }
  virtual ~Schedule() = default;

  nxs_int getDeviceId() const { return device_id; }

  void addCommand(Tcommand *command) { commands.push_back(command); }

  const Commands &getCommands() const { return commands; }

  virtual nxs_status run(Tstream stream, nxs_uint run_settings) = 0;

  virtual nxs_status release() {
    commands.clear();
    return NXS_Success;
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_SCHEDULE_H