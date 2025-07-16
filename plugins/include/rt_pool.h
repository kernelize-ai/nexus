#ifndef RT_POOL_H
#define RT_POOL_H

#include <nexus-api.h>

#include <cassert>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

namespace nxs {
namespace rt {

/**
 * Template class for object pooling
 * Provides efficient allocation and deallocation of objects by reusing them
 * Pool owns all objects and manages them in vector-based storage
 */
template <typename T>
class Pool {
 private:
  std::vector<T> object_storage_;           // Owns all objects
  std::vector<nxs_int> available_indices_;  // Indices of available objects
  std::mutex pool_mutex_;

 public:
  /**
   * Constructor
   * @param initial_capacity Initial capacity for the pool
   */
  explicit Pool(size_t initial_capacity = 1024) {
    // Pre-allocate storage
    object_storage_.reserve(initial_capacity);
  }

  /**
   * Get an object from the pool
   * @return Raw pointer to an object (pool maintains ownership)
   */
  template <typename... Args>
  nxs_int acquire(Args&&... args) {
    std::lock_guard<std::mutex> lock(pool_mutex_);

    // First try to reuse an available object
    if (!available_indices_.empty()) {
      nxs_int index = available_indices_.back();
      available_indices_.pop_back();
      return index;
    }

    if (object_storage_.size() >= object_storage_.capacity()) {
      object_storage_.reserve(object_storage_.capacity() + 1024);
    }
    // Create new object (no size limit)
    nxs_int index = object_storage_.size();
    object_storage_.emplace_back(std::forward<Args>(args)...);
    return index;
  }

  template <typename... Args>
  T* get_new(Args&&... args) {
    nxs_int index = acquire(std::forward<Args>(args)...);
    return &object_storage_[index];
  }

  /**
   * Return an object to the pool
   * @param obj Pointer to object to release
   */
  void release(T* obj) {
    if (!obj) return;

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Find the index of the object
    nxs_int index = obj - &object_storage_[0];
    if (index < object_storage_.size()) {
      available_indices_.push_back(index);
    }
  }

  /**
   * Return an object to the pool
   * @param index Index of object to release
   */
  void release(nxs_int index) {
    if (index < 0 || index >= object_storage_.size()) return;
    std::lock_guard<std::mutex> lock(pool_mutex_);
    available_indices_.push_back(index);
  }

  T* get(nxs_int index) {
    if (index < 0 || index >= object_storage_.size()) return nullptr;
    return &object_storage_[index];
  }

  /**
   * Get current pool statistics
   * @return Pair of (available objects, total objects)
   */
  std::pair<size_t, size_t> get_stats() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return {available_indices_.size(), object_storage_.size()};
  }

  /**
   * Clear all objects from the pool
   */
  void clear() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    object_storage_.clear();
    available_indices_.clear();
  }

  /**
   * Reserve capacity for the pool
   * @param capacity New capacity to reserve
   */
  void reserve(size_t capacity) {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    object_storage_.reserve(capacity);
  }

  /**
   * Get current capacity of the pool
   * @return Current capacity
   */
  size_t capacity() const { return object_storage_.capacity(); }

  /**
   * Get total number of objects currently in use
   * @return Number of objects in use
   */
  size_t get_in_use_count() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return object_storage_.size() - available_indices_.size();
  }

  /**
   * Check if an object belongs to this pool
   * @param obj Pointer to check
   * @return True if object belongs to this pool
   */
  bool owns_object(const T* obj) {
    if (!obj) return false;
    std::lock_guard<std::mutex> lock(pool_mutex_);
    return obj >= &object_storage_[0] &&
           obj < &object_storage_[0] + object_storage_.size();
  }
};

}  // namespace rt
}  // namespace nxs

#endif  // RT_POOL_H