#ifndef RT_SCHEDULE_H
#define RT_SCHEDULE_H

#include <rt_command.h>

namespace nxs {
namespace rt {

template <typename Tcommand, typename Tdevice, typename Tstream>
class Schedule {
  typedef std::vector<Tcommand *> Commands;

  Tdevice device;
  nxs_uint settings;
  Commands commands;

 public:
  Schedule(Tdevice dev = Tdevice(), nxs_uint settings = 0)
      : device(dev), settings(settings) {
    commands.reserve(8);
  }
  virtual ~Schedule() = default;

  Tdevice getDevice() const { return device; }

  nxs_uint getSettings() const { return settings; }

  void addCommand(Tcommand *command) { commands.push_back(command); }

  const Commands &getCommands() const { return commands; }

  virtual nxs_status run(Tstream stream, nxs_uint run_settings) = 0;

  virtual nxs_status release() {
    device = Tdevice();
    commands.clear();
    return NXS_Success;
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_SCHEDULE_H