# EchoNexus

EchoNexus is a lightweight HTTP framework for modern C++23.

## Quick Start

### Requirements

- A C++23-capable compiler
- CMake 3.25 or newer
- Git, if you want EchoNexus to bootstrap a local `vcpkg` checkout automatically
- [`Just`](https://just.systems) — a command runner used for the common workflows below

When EchoNexus is configured as the top-level project, it can reuse `VCPKG_ROOT`, an explicit `CMAKE_TOOLCHAIN_FILE`, or clone and bootstrap `vcpkg` into `.vcpkg/` for you.

### Build the example

```bash
git clone https://github.com/Cosmic-Stars-Team/EchoNexus.git
cd EchoNexus

just run
```

Once the example is running, open `http://127.0.0.1:9000` in your browser.

> **Windows users**: MSVC is supported via [`just`](https://just.systems) (requires installing `just` first).
> Open a **Developer Command Prompt for VS** and run:
>
> ```cmd
> cd EchoNexus
> just build          # configure + build (debug)
> just test           # run tests
> just run            # build & run the example
> ```
>
> Use `just build --release` for a release build.

You can also try a few routes directly:

```bash
curl http://127.0.0.1:9000/
curl http://127.0.0.1:9000/api/info
curl http://127.0.0.1:9000/api/v1/user/42
curl http://127.0.0.1:9000/api/v1/user/42/info
```

## Using EchoNexus In Your Project

Today, the simplest integration path is to vendor the repository and add it to your CMake build with `add_subdirectory`.

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyApp LANGUAGES CXX)

add_subdirectory(third_party/EchoNexus)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE EchoNexus::EchoNexus)
target_compile_features(my_app PRIVATE cxx_std_23)
```

If you already manage dependencies with `vcpkg`, point `CMAKE_TOOLCHAIN_FILE` at your existing setup. If you do not, EchoNexus can bootstrap a local copy when built as the top-level project.

## Core Concepts

### `echo::nexus`

`nexus` is the application entry point. It owns the top-level handler chain and delegates HTTP transport to an executor. The built-in executor is `echo::beast_executor`, which runs the server on top of Boost.Beast and Boost.Asio.

### Middleware and handlers

EchoNexus models request processing as a chain of middleware. Each handler receives a shared request object plus an optional `next` function. A handler can return immediately, mutate the request, or call `next` to continue through the pipeline.

The same model is used for custom layers, routers, logging, and CORS, which keeps the composition rules predictable.

### Request context

Each request carries a context map backed by `std::any`. Middleware can attach data with `set_ctx(...)`, and downstream handlers can read it back with `get_ctx<T>(...)`. This is how routed path parameters and custom middleware state move through the pipeline.

### Responses

`echo::type::response` is a plain response object with helpers for common HTTP use cases:

- `response::text(...)`
- `response::html(...)`
- `response::json(...)`
- `response::redirect(...)`

Headers can be set directly, and `Content-Length` is updated automatically when the body changes.

## Built-In Middleware

EchoNexus currently ships with two middleware components in the repository:

- `logger`, a simple request logger that records method, path, status, latency, and client IP
- `cors`, a configurable CORS layer with `strict`, `permissive`, and `development` presets plus explicit origin, method, header, and credential controls
- `router`, handles method-based routing, route-level middleware, router nesting, and dynamic path segments.

These are intended to cover common cases without hiding the underlying request flow.

## Build Notes

- EchoNexus requires C++23.
- The repository currently depends on `Boost::asio`, `glaze`, and, by default, `Boost.Beast`.
- Setting `ECHONEXUS_DISABLE_BEAST=ON` removes the built-in Beast-backed executor. In that mode, the library can still be used with a custom executor, but the bundled example server is not available.
- `ECHONEXUS_WARNINGS_AS_ERRORS` is enabled by default.
- `ECHONEXUS_BUILD_EXAMPLES` is enabled automatically when EchoNexus is the top-level project.

## Project Layout

- [include/](/Users/leo/Documents/Development/Contribute/EchoNexus/include) contains the public headers, including the middleware, request, response, and server interfaces
- [src/](/Users/leo/Documents/Development/Contribute/EchoNexus/src) contains the framework implementation
- [example/](/Users/leo/Documents/Development/Contribute/EchoNexus/example) contains a runnable sample application
- [CMakeLists.txt](/Users/leo/Documents/Development/Contribute/EchoNexus/CMakeLists.txt) defines the build, dependency setup, and install/export rules
