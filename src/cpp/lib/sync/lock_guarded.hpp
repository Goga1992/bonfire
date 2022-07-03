#pragma once

#include <mutex>
#include <optional>

namespace bon::sync
{

template <typename T>
class LockGuarded
{
 public:
  class Access
  {
   public:
    Access(std::mutex& mtx, T& obj) : lock(mtx), obj_ref(obj) {}

   public:
    T& operator*() { return obj_ref; }
    const T& operator*() const { return obj_ref; }

   private:
    std::scoped_lock<std::mutex> lock;
    T& obj_ref;
  };

 public:
  LockGuarded() = default;

  LockGuarded(T&& other) : obj(std::move(other)) {}
  LockGuarded(const T& other) : obj(other) {}

  LockGuarded(const LockGuarded&) = delete;
  LockGuarded& operator=(const LockGuarded&) = delete;

 public:
  Access Get() { return Access(mtx, obj); }

 private:
  std::mutex mtx;
  T obj;
};

}  // namespace bon::sync