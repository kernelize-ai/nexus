#ifndef NEXUS_UTILITY_H
#define NEXUS_UTILITY_H

#include <functional>
#include <vector>
#include <string>

namespace nexus {

    typedef std::function<void (const std::string &, const std::string &)> PathNameFn;
    void iterateEnvPaths(const char *envVar, const char *envDefault, const PathNameFn &func);

}

#endif // NEXUS_SYSTEM_H