#include <print>
#include <thread>

#include <asio.hpp>

static std::atomic_int tid_gen;
thread_local int const tid = tid_gen++;

inline void out(auto const& msg) {
  std::println("T{:x} {}", tid, msg);
}

asio::awaitable<void> f(auto strand) {
  out("IO Context Executor");
  co_await dispatch(bind_executor(strand, asio::deferred));
  out("Strand Executor");
  co_await dispatch(asio::deferred);
  out("Back on IO Context Executor");
}

int main() {
  asio::io_context io;
  asio::thread_pool tp(1);

  co_spawn(io, f(make_strand(tp)), asio::detached);

  io.run();
}
