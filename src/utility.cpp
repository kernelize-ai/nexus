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

// Base64 decoding utility
std::vector<uint8_t> nexus::base64Decode(const std::string_view& encoded,
                                         size_t decoded_size) {
  static const std::string chars =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  // Assume encoded is clean
#if 0
  // TODO: clean inplace
  std::string clean_input = encoded;
  // Remove whitespace and newlines
  clean_input.erase(std::remove_if(clean_input.begin(), clean_input.end(), 
                                  [](unsigned char c) { return std::isspace(c); }), 
                    clean_input.end());

  // Remove padding
  while (!clean_input.empty() && clean_input.back() == '=') {
      clean_input.pop_back();
  }
#endif

  int encoded_size = encoded.size();
  while (encoded[encoded_size - 1] == '=') {
    encoded_size--;
  }

  std::vector<uint8_t> decoded;
  decoded.reserve(encoded_size * 3 / 4);

  for (size_t i = 0; i < encoded_size; i += 4) {
    uint32_t tmp = 0;
    int valid_chars = 0;

    for (int j = 0; j < 4 && (i + j) < encoded_size; ++j) {
      char c = encoded[i + j];
      auto pos = chars.find(c);
      if (pos != std::string::npos) {
        tmp = (tmp << 6) | pos;
        valid_chars++;
      }
    }

    if (valid_chars >= 2) {
      decoded.push_back((tmp >> 16) & 0xFF);
      if (valid_chars >= 3) {
        decoded.push_back((tmp >> 8) & 0xFF);
        if (valid_chars >= 4) {
          decoded.push_back(tmp & 0xFF);
        }
      }
    }
  }
  // assert(decoded.size() >= decoded_size);
  if (decoded_size > 0) {
    decoded.resize(decoded_size);
  }
  return decoded;
}
