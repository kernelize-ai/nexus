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
template <typename T, size_t chunk_size = 1024>
class Pool {
 private:
  typedef std::array<T, chunk_size> Chunk;
  std::vector<Chunk> object_storage_;       // Owns all objects
  std::vector<nxs_int> available_indices_;  // Indices of available objects
  std::mutex pool_mutex_;
  nxs_int tail_index_;

  std::pair<nxs_int, nxs_int> getIndexPair(nxs_int index) {
    if (index < 0) return {-1, -1};
    return {index / chunk_size, index % chunk_size};
  }

  Chunk& getChunk(nxs_int index) { return object_storage_[index]; }

 public:
  /**
   * Constructor
   * @param initial_capacity Initial capacity for the pool
   */
  explicit Pool() : tail_index_(0) { object_storage_.push_back(Chunk()); }

  ~Pool() { clear(); }

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
      auto [chunk_index, chunk_offset] = getIndexPair(index);
      auto& chunk = getChunk(chunk_index);
      new (&chunk[chunk_offset]) T(std::forward<Args>(args)...);
      return index;
    }

    auto [chunk_index, chunk_offset] = getIndexPair(tail_index_);
    if (chunk_index >= object_storage_.size()) {
      object_storage_.push_back(Chunk());
    }
    auto& chunk = getChunk(chunk_index);
    new (&chunk[chunk_offset]) T(std::forward<Args>(args)...);
    return tail_index_++;
  }

  template <typename... Args>
  T* get_new(Args&&... args) {
    nxs_int index = acquire(std::forward<Args>(args)...);
    return get(index);
  }

  /**
   * Return an object to the pool
   * @param obj Pointer to object to release
   */
  void release(T* obj) {
    if (!obj) return;

    std::lock_guard<std::mutex> lock(pool_mutex_);

    // Find the index of the object
    nxs_int chunk_index = 0;
    for (auto& chunk : object_storage_) {
      auto chunk_offset = obj - &chunk[0];
      if (chunk_offset >= 0 && chunk_offset < chunk_size) {
        // obj->~T();
        available_indices_.push_back(chunk_index * chunk_size + chunk_offset);
        return;
      }
      chunk_index++;
    }
  }

  /**
   * Return an object to the pool
   * @param index Index of object to release
   */
  void release(nxs_int index) {
    if (index < 0 || index >= tail_index_) return;
    std::lock_guard<std::mutex> lock(pool_mutex_);
    auto* obj = get(index);
    // if (obj) obj->~T();
    available_indices_.push_back(index);
  }

  T* get(nxs_int index) {
    if (index < 0 || index >= tail_index_) return nullptr;
    auto [chunk_index, chunk_offset] = getIndexPair(index);
    auto& chunk = getChunk(chunk_index);
    return &chunk[chunk_offset];
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
  size_t capacity() const { return tail_index_; }

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