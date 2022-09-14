#include "context.h"

// #define USE_DEBUG
#include "common/error.h"
#include "common/log.h"

namespace cppgo {

void Worker::start() {
  thread_ = std::thread([this]() {
    auto tid = this->id();
    status_ = Status::RUNNING;
    DEBUG("worker {%u} start", tid);
    while (!ctx_->done()) {
      Context::TaskPointer current = nullptr;
      {
        std::unique_lock guard(ctx_->mtx_);
        if (!ctx_->runnable_set_.empty()) {
          current = *ctx_->runnable_set_.begin();
          ctx_->runnable_set_.erase(current);
        }
      }
      if (!current) {
        DEBUG("worker {%u} get no task, sleep", tid);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        DEBUG("worker {%u} sleep end, resume", tid);
        continue;
      }
      DEBUG("worker {%u} get task {%u}", tid, current->id());

      {
        std::unique_lock guard(ctx_->mtx_);
        ctx_->running_map_[current] = this;
        DEBUG("worker {%u} bind with {%u}", tid, current->id());
      }

      DEBUG("worker {%u} start execute task {%u}", tid, current->id());
      current->resume();
      DEBUG("worker {%u} execute task {%u} end", tid, current->id());

      {
        std::unique_lock guard(ctx_->mtx_);
        ctx_->running_map_.erase(current);
        DEBUG("worker {%u} unbind with {%u}", tid, current->id());
      }

      {
        std::unique_lock guard(ctx_->mtx_);

        ASSERT(!(ctx_->runnable_set_.count(current)) &&
                   !(ctx_->blocked_set_.count(current)),
               "current task should not in statu sets");

        if (current->done()) {
          DEBUG("task {%u} done, resource clean", current->id());
          ctx_->tasks_.destroy(current);
          continue;
        }
        if (current->callback_) {
          // use current's callback to update current's status
          DEBUG("worker {%u} start callback task {%u}", tid, current->id());
          current->callback_();
          current->callback_ = nullptr;
          DEBUG("worker {%u} callback task {%u} end", tid, current->id());
        } else {
          // set current to runnable
          ctx_->runnable_set_.insert(current);
          current->status_ = Task::Status::RUNNABLE;
          DEBUG("worker {%u} set task {%u} to runnable", tid, current->id());
        }
      }
    }
    status_ = Status::DONE;
    DEBUG("worker {%u} end", tid);
  });
  DEBUG("thread_ id=%u", thread_.get_id());
}

void Worker::join() {
  if (thread_.joinable()) {
    thread_.join();
  }
}

}  // namespace cppgo
