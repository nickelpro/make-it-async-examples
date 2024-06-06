#include <array>
#include <bit>
#include <concepts>
#include <csignal>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <format>
#include <functional>
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
using asio::dispatch;
using asio::execution::executor;
using asio::ip::tcp;

struct PyResult {
  PyObject* obj;
  char* out;
  Py_ssize_t len;
};

template <executor Ex> struct PythonApp {
  PythonApp(Ex ex, PyObject* app)
      : ex_ {std::move(ex)}, app_ {app}, vecCall_ {PyVectorcall_Function(app)} {
  }

  template <typename CT>
  auto run(const std::vector<char>& data, PyObject* prev, CT&& token) {
    auto init = [this, &data, prev](auto handler) {
      dispatch(ex_, [this, prev, &data, h = std::move(handler)]() mutable {
        dispatch(asio::append(std::move(h), run_impl(data, prev)));
      });
    };

    return asio::async_initiate<CT, void(PyResult)>(init, token);
  }

  template <typename CT> auto decref_pyobj(PyObject* obj, CT&& token) {
    auto init = [this, obj](auto handler) {
      dispatch(ex_, [this, obj, h = std::move(handler)]() mutable {
        decref_pyobj_impl(obj);
        dispatch(std::move(h));
      });
    };
    return asio::async_initiate<CT, void()>(init, token);
  }

private:
  PyResult run_impl(const std::vector<char>& data, PyObject* prev) {
    Py_XDECREF(prev);
    PyResult ret {};

    auto pyBytes {PyBytes_FromStringAndSize(data.data(), data.size())};
    if(!pyBytes)
      return ret;


    ret.obj = vecCall_ ? vecCall_(app_, &pyBytes, 1, NULL)
                       : PyObject_CallOneArg(app_, pyBytes);
    Py_DECREF(pyBytes);

    if(!ret.obj) {
      PyErr_Print();
      PyErr_Clear();
      return ret;
    }

    if(PyBytes_AsStringAndSize(ret.obj, &ret.out, &ret.len)) {
      PyErr_Print();
      PyErr_Clear();
      Py_CLEAR(ret.obj);
    }

    return ret;
  }

  void decref_pyobj_impl(PyObject* obj) {
    Py_DECREF(obj);
  }

  Ex ex_;
  PyObject* app_;
  vectorcallfunc vecCall_;
};

constexpr auto nbeswap(std::integral auto val) noexcept {
  if constexpr(std::endian::native == std::endian::big)
    return val;
  return std::byteswap(val);
}

asio::awaitable<void> client_handler(tcp::socket s, auto& app) {
  PyResult pr {};
  try {
    std::vector<char> data;
    for(;;) {
      std::uint16_t hdr;
      co_await asio::async_read(s, buffer(&hdr, sizeof(hdr)), deferred);

      data.resize(nbeswap(hdr));
      co_await asio::async_read(s, buffer(data), deferred);

      pr = co_await app.run(data, pr.obj, deferred);
      if(!pr.obj)
        break;

      hdr = nbeswap(static_cast<std::uint16_t>(pr.len));
      std::array seq {buffer(&hdr, sizeof(hdr)), buffer(pr.out, pr.len)};
      co_await asio::async_write(s, seq, deferred);
    }
  } catch(...) {}

  if(pr.obj)
    co_await app.decref_pyobj(pr.obj, deferred);
}

asio::awaitable<void> listener(tcp::endpoint ep, auto& app) {
  auto executor {co_await asio::this_coro::executor};
  tcp::acceptor acceptor {executor, ep};

  for(;;) {
    tcp::socket socket {co_await acceptor.async_accept(deferred)};
    asio::co_spawn(executor, client_handler(std::move(socket), app), detached);
  }
}

void accept(executor auto ex, std::string_view host, std::string_view port,
    auto& app) {
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

  PyObject* appObj;
  const char* host = "localhost";
  const char* port = "8000";

  if(!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &parser, &appObj,
         &host, &port))
    return nullptr;

  Py_IncRef(appObj);

  auto old_sigint {std::signal(SIGINT, SIG_DFL)};

  asio::io_context io {1};
  asio::signal_set signals {io, SIGINT};
  signals.async_wait([&](auto, auto) {
    PyErr_SetInterrupt();
    io.stop();
  });

  PythonApp app {asio::make_strand(io), appObj};
  accept(io.get_executor(), host, port, app);
  io.run();

  signals.cancel();
  signals.clear();
  std::signal(SIGINT, old_sigint);

  Py_DecRef(appObj);
  Py_RETURN_NONE;
}

static PyMethodDef Figure4Methods[] {
    {"run", (PyCFunction) run, METH_FASTCALL | METH_KEYWORDS},
    {0},
};

static PyModuleDef Figure4Module {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "Figure4",
    .m_doc = "Example of a server that delegates to a Python function",
    .m_size = -1,
    .m_methods = Figure4Methods,
};

PyMODINIT_FUNC PyInit_Figure4(void) {
  auto mod {PyModule_Create(&Figure4Module)};
  if(!mod || PyModule_AddStringConstant(mod, "__version__", "1.0.0"))
    return nullptr;
  return mod;
}
