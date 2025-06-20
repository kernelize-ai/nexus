#ifndef RUNTIME_SCHEDULE_H
#define RUNTIME_SCHEDULE_H

#include <string>
#include <vector>

#include <runtime_command.h>

class RuntimeSchedule {

public:

    RuntimeSchedule() = default;
    ~RuntimeSchedule() = default;

    Commands commands;
};

typedef std::vector<RuntimeSchedule> Schedules;

#endif // RUNTIME_SCHEDULE_H