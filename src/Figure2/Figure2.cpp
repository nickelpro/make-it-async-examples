#include <array>
#include <bit>
#include <concepts>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <print>
#include <string_view>
#include <utility>
#include <vector>

#include <asio.hpp>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

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

asio::awaitable<void> client_handler(tcp::socket s, PyObject* app) {
  PyObject* appReturn {nullptr};
  try {
    std::vector<char> data;
    auto vecCall {PyVectorcall_Function(app)};
    for(;;) {
      std::uint16_t hdr;
      co_await asio::async_read(s, buffer(&hdr, sizeof(hdr)), deferred);

      data.resize(nbeswap(hdr));
      co_await asio::async_read(s, buffer(data), deferred);

      auto state {PyGILState_Ensure()};
      Py_XDECREF(appReturn);

      auto pyBytes {PyBytes_FromStringAndSize(data.data(), data.size())};
      if(!pyBytes) {
        PyGILState_Release(state);
        break;
      }

      appReturn = vecCall ? vecCall(app, &pyBytes, 1, NULL)
                          : PyObject_CallOneArg(app, pyBytes);
      Py_DECREF(pyBytes);

      if(!appReturn) {
        PyErr_Print();
        PyErr_Clear();
        PyGILState_Release(state);
        break;
      }

      char* out;
      Py_ssize_t len;
      if(PyBytes_AsStringAndSize(appReturn, &out, &len)) {
        PyErr_Print();
        PyErr_Clear();
        Py_DECREF(appReturn);
        PyGILState_Release(state);
        break;
      }

      PyGILState_Release(state);

      hdr = nbeswap(static_cast<std::uint16_t>(len));

      std::array seq {buffer(&hdr, sizeof(hdr)), buffer(out, len)};
      co_await asio::async_write(s, seq, deferred);
    }
  } catch(...) {
    auto state {PyGILState_Ensure()};
    Py_XDECREF(appReturn);
    PyGILState_Release(state);
  }
}

asio::awaitable<void> listener(tcp::endpoint ep, PyObject* app) {
  auto executor {co_await asio::this_coro::executor};
  tcp::acceptor acceptor {executor, ep};

  for(;;) {
    tcp::socket socket {co_await acceptor.async_accept(deferred)};
    asio::co_spawn(executor, client_handler(std::move(socket), app), detached);
  }
}

void accept(executor auto ex, std::string_view host, std::string_view port,
    PyObject* app) {
  for(auto re : tcp::resolver {ex}.resolve(host, port)) {
    auto ep {re.endpoint()};
    std::println("Listening on: {}:{}", ep.address().to_string(), ep.port());
    asio::co_spawn(ex, listener(std::move(ep), app), detached);
  }
}

PyObject* run(PyObject* self, PyObject* const* args, Py_ssize_t nargs,
    PyObject* kwnames) {
  static const char* keywords[] {"app", "host", "port"};
  static _PyArg_Parser parser {.format = "O|ss:run", .keywords = keywords};

  PyObject* app;
  const char* host = "localhost";
  const char* port = "8000";

  if(!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &parser, &app, &host,
         &port))
    return nullptr;

  Py_IncRef(app);

  auto old_sigint {std::signal(SIGINT, SIG_DFL)};

  asio::io_context io {1};
  asio::signal_set signals {io, SIGINT};
  signals.async_wait([&](auto, auto) {
    PyErr_SetInterrupt();
    io.stop();
  });

  accept(io.get_executor(), host, port, app);

  PyThreadState* thread {PyEval_SaveThread()};
  io.run();

  signals.cancel();
  signals.clear();
  std::signal(SIGINT, old_sigint);

  PyEval_RestoreThread(thread);
  Py_DecRef(app);
  Py_RETURN_NONE;
}

static PyMethodDef Figure2Methods[] {
    {"run", (PyCFunction) run, METH_FASTCALL | METH_KEYWORDS},
    {0},
};

static PyModuleDef Figure2Module {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "Figure2",
    .m_doc = "Example of a server that delegates to a Python function",
    .m_size = -1,
    .m_methods = Figure2Methods,
};

PyMODINIT_FUNC PyInit_Figure2(void) {
  auto mod {PyModule_Create(&Figure2Module)};
  if(!mod || PyModule_AddStringConstant(mod, "__version__", "1.0.0"))
    return nullptr;
  return mod;
}
