#ifndef NEXUS_LOG_H
#define NEXUS_LOG_H

#define NEXUS_LOG_DEPTH 30

#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>

namespace nexus {

enum nxs_log_severity {
  NXS_LOG_ERROR = 0,
  NXS_LOG_NOTE = 1,
  NXS_LOG_WARN = 2,
};
  
// Singleton LogManager that owns the log file
class LogManager {
public:
  static LogManager& getInstance() {
    static LogManager instance;
    return instance;
  }

  void setOpen(bool open) {
    if (open) {
      const char* logPath = std::getenv("NEXUS_LOG_FILE");
      std::string filename = logPath ? logPath : "nexus.log";
      setLogFile(filename);
    } else {
      std::lock_guard<std::mutex> lock(mutex_);
      logFile_.close();
    }
  }
  bool isOpen() const {
    return logFile_.is_open();
  }

  template<typename... Args>
  void log(nxs_log_severity severity, const char *module, Args&&... args) {
    if (logFile_.is_open()) {
      std::lock_guard<std::mutex> lock(mutex_);
      const char *severity_str[] = { "ERROR", "NOTE", "WARN" };
      logFile_ << "[" << module << "]" << severity_str[severity]
               << std::left << std::setw(NEXUS_LOG_DEPTH) << ": ";
      ((logFile_ << args), ...);
      logFile_ << std::endl;
    }
  }
  
  void setLogFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (logFile_.is_open()) {
      logFile_.close();
    }
    logFile_.open(filename, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
      std::cerr << "Failed to open log file: " << filename << std::endl;
    }
  }
  
  // Disable copy and move
  LogManager(const LogManager&) = delete;
  LogManager& operator=(const LogManager&) = delete;
  LogManager(LogManager&&) = delete;
  LogManager& operator=(LogManager&&) = delete;

private:
  LogManager() {
    const char* logPath = std::getenv("NEXUS_LOG_ENABLE");
    bool enable = logPath ? std::stoi(logPath) : false;
    setOpen(enable);
  }
  
  ~LogManager() { setOpen(false); }
  
  std::ofstream logFile_;
  std::mutex mutex_;
};

} // namespace nexus

#define NEXUS_LOG(SEVERITY, ...) \
  ::nexus::LogManager::getInstance().log(SEVERITY, NEXUS_LOG_MODULE, __VA_ARGS__)

#endif  // NEXUS_LOG_H