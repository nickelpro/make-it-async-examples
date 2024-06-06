#include <array>
#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <format>
#include <print>
#include <string_view>
#include <utility>
#include <vector>

#include <asio.hpp>

using asio::buffer;
using asio::deferred;
using asio::detached;
using asio::execution::executor;
using asio::ip::tcp;

constexpr auto nbeswap(std::integral auto val) noexcept {
  if constexpr(std::endian::native == std::endian::big)
    return val;
  return std::byteswap(val);
}

asio::awaitable<void> client_handler(tcp::socket s) {
  try {
    std::vector<char> data;
    for(;;) {
      uint16_t hdr;
      co_await asio::async_read(s, buffer(&hdr, sizeof(hdr)), deferred);

      data.resize(nbeswap(hdr));
      co_await asio::async_read(s, buffer(data), deferred);

      std::array seq {buffer(&hdr, sizeof(hdr)), buffer(data)};
      co_await asio::async_write(s, seq, deferred);
    }
  } catch(...) {}
}

asio::awaitable<void> listener(tcp::endpoint ep) {
  auto executor {co_await asio::this_coro::executor};
  tcp::acceptor acceptor {executor, ep};

  for(;;) {
    tcp::socket socket {co_await acceptor.async_accept(deferred)};
    asio::co_spawn(executor, client_handler(std::move(socket)), detached);
  }
}

void accept(executor auto ex, std::string_view host, std::string_view port) {
  for(auto re : tcp::resolver {ex}.resolve(host, port)) {
    auto ep {re.endpoint()};
    std::println("Listening on: {}:{}", ep.address().to_string(), ep.port());
    asio::co_spawn(ex, listener(std::move(ep)), detached);
  }
}

std::pair<std::string_view, std::string_view> get_host_port(char* option) {
  char* colon {strchr(option, ':')};
  if(colon)
    return {{option, (unsigned) (colon - option)}, colon + 1};
  return {option, ""};
}

int main(int argc, char* argv[]) {
  if(argc > 2) {
    std::println("Usage: {} [host[:port]]", argv[0]);
    std::exit(EXIT_FAILURE);
  }

  if(argc > 1 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
    std::println("Usage: {} [host[:port]]", argv[0]);
    std::exit(EXIT_SUCCESS);
  }

  asio::io_context io {1};

  if(argc > 1) {
    const auto [host, port] {get_host_port(argv[1])};
    accept(io.get_executor(), host, port);
  } else {
    accept(io.get_executor(), "localhost", "8000");
  }

  io.run();
}
