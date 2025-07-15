#include <nexus/log.h>
#include <nexus/utility.h>

#include <filesystem>
#include <iostream>
#include <sstream>

using namespace nexus;

#define NEXUS_LOG_MODULE "utility"

static std::vector<std::string> splitPaths(const std::string& paths,
                                           char delimiter) {
  std::vector<std::string> result;
  std::stringstream ss(paths);
  std::string path;
  while (std::getline(ss, path, delimiter)) {
    result.push_back(path);
  }
  return result;
}

void nexus::iterateEnvPaths(const char* envVar, const char* envDefault,
                            const nexus::PathNameFn& func) {
  // Load Runtimes from NEXUS_DEVICE_PATH
  const char* env = std::getenv(envVar);
  if (!env) {
    NEXUS_LOG(NEXUS_STATUS_WARN, envVar << " environment variable is not set.");
    env = envDefault;
  }

  std::vector<std::string> directories = splitPaths(env, ':');
  for (const auto& dirname : directories) {
    try {
      std::filesystem::path directory(dirname);

      NEXUS_LOG(NEXUS_STATUS_NOTE, "Reading directory: " << directory);
      for (auto const& dir_entry :
          std::filesystem::directory_iterator{directory}) {
        if (dir_entry.is_regular_file()) {
          auto filepath = dir_entry.path();
          NEXUS_LOG(NEXUS_STATUS_NOTE, "  Adding file: " << filepath);

          func(filepath, filepath.filename());
          }
        }
    } catch (std::filesystem::filesystem_error const& ex) {
      NEXUS_LOG(NEXUS_STATUS_ERR, "Error iterating environment paths: " << ex.what());
    }
  }
}
