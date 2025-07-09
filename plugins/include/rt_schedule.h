#ifndef RT_SCHEDULE_H
#define RT_SCHEDULE_H

#include <string>
#include <vector>

#include <rt_command.h>

namespace nxs {
namespace rt {

class Schedule : public rt::Object {

public:

  Commands commands;

  Schedule(Object *device = nullptr) : Object(device) {}
  ~Schedule() = default;

  void insertCommand(rt::Command *command) {
    commands.push_back(command);
  }

  Commands getCommands() {
    return commands;
  }

};

}  // namespace rt
}  // namespace nxs

#endif // RT_SCHEDULE_H