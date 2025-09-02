#ifndef NEXUS_UTILITY_H
#define NEXUS_UTILITY_H

#include <functional>
#include <string>
#include <vector>

namespace nexus {

typedef std::function<void(const std::string &, const std::string &)>
    PathNameFn;
void iterateEnvPaths(const char *envVar, const char *envDefault,
                     const PathNameFn &func);

std::vector<uint8_t> base64Decode(const std::string_view &encoded,
                                  size_t decoded_size);

}  // namespace nexus

#endif  // NEXUS_SYSTEM_H