#pragma once

#include <any>
#include <coroutine>
#include <memory>

#include "util/unwrapper.h"

namespace cppgo {

class PromiseBase {
  template <__detail::HasImpl T>
  friend typename T::Impl& __detail::impl(T& wrapper);

 public:
  PromiseBase();
  PromiseBase(PromiseBase&) = delete;
  PromiseBase(PromiseBase&&) = default;
  virtual ~PromiseBase();

 public:
  void init(std::coroutine_handle<> handler);
  std::suspend_always initial_suspend() { return {}; }
  std::suspend_always final_suspend() noexcept { return {}; }
  void unhandled_exception() noexcept;
  std::any& any();

 protected:
  std::suspend_always _yield_any(std::any&& value);

 public:
  class Impl;

 private:
  std::unique_ptr<Impl> _impl;
};

template <typename T>
class Promise : public PromiseBase {
 public:
  std::suspend_always yield_value(T&& value) { return this->_yield_any(value); }
  void return_value(T&& value) { this->_yield_any(std::move(value)); }
};

template <>
class Promise<void> : public PromiseBase {
 public:
  void return_void() {}
};

}  // namespace cppgo
