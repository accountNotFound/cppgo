#pragma once

#include <functional>
#include <queue>
#include <unordered_set>

#include "header.h"
#include "common/spinlock.h"
#include "common/time_order_set.h"
#include "coroutine/coroutine.h"

namespace cppgo {

struct Resource;
class Monitor;

struct Resource {
  friend class Monitor;

 public:
  Resource(size_t capacity) : capacity_(capacity) {}
  Resource(Resource&) = delete;

  void lock() { self_.lock(); }
  void unlock() { self_.unlock(); }

 private:
  // call with lock
  bool available_() { return use_count_ < capacity_; }
  // call with lock
  bool acquire_() { return use_count_ < capacity_ ? ++use_count_ : 0; }
  // call with lock
  void release_() { --use_count_; }

 private:
  SpinLock self_;
  size_t capacity_;
  size_t use_count_ = 0;
};

class Monitor {
  friend class Context;
  friend class Worker;
  friend class Task;

 public:
  using Id = size_t;

 public:
  Monitor(Context* ctx, Resource* resource);
  Monitor(Monitor&) = delete;

  Id id() { return id_; }

  AsyncFunction<void> enter();
  void exit();

  // call with Resource's lock
  void notify_one_with_guard();
  // call with Resource's lock, automatically unlock when co_return
  AsyncFunction<void> suspend_with_guard_unlock();

 private:
  // just for unique id generation
  static SpinLock cls_;
  static Id cls_id_;

  Id id_;

  Context* ctx_;

  // this set is protected by Context's mutex in Task's callback
  TimeOrderSet<Task*> blocked_set_;

  Resource* resource_;
};

}  // namespace  cppgo