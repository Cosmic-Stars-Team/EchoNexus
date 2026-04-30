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
> just build          # build the library + example (debug)
> just clean          # remove CMake caches and build artifacts
> just test           # run tests
> just run            # build & run the example
> ```
>
> Use `just build --release` for a release build.

### Run tests

```bash
just clean             # remove CMake caches and build artifacts
just build             # build the library + example
just test              # run the full test suite
just test unit         # run only unit tests
just test integration  # run only integration tests
just test --export ./test-results.xml
just test integration --export ./integration-results.xml
```

Add `--release` if you want to run the release build instead of the default debug build.
Use `--export <path>` to write JUnit XML for the current `ctest` run.
Use `--compiler <cxx>` with either command to select a toolchain explicitly, for example `just build --compiler /usr/bin/clang++ --release` or `just test unit --compiler g++-15`.
When `--compiler` is set, EchoNexus derives the matching C compiler and forwards both `CC` and `CXX` via `cmake -E env`, so CMake and `vcpkg` stay on the same toolchain. Supported compiler basenames are `cc`, `c++`, `g++*`, `clang++*`, `cl`, and `clang-cl`; the generic `cc` / `c++` pair is resolved to sibling drivers automatically.

### Run benchmarks

The repository includes a native benchmark suite under [benchmarks/GUIDE.md](/Users/leo/Documents/Development/Contribute/EchoNexus/benchmarks/GUIDE.md). It compares EchoNexus against FastAPI, Flask, Koa, Elysia, Axum, Gin, and Spring Boot with Bun as the runner and `oha` as the load generator.

Typical flow:

```bash
just benchmark smoke
just benchmark --frameworks echonexus,axum --workloads plaintext,time_json --workers 1,4 --concurrency 50,200,500 --release
```

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

When EchoNexus is used via `add_subdirectory`, dependency discovery is owned by the parent project. Set `CMAKE_TOOLCHAIN_FILE` (or otherwise make `Boost` and `glaze` discoverable) before the parent project's first `project()` call.

EchoNexus only auto-bootstraps a local `vcpkg` checkout when it is itself the top-level configure entry point.

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

EchoNexus currently ships with three middleware components in the repository:

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
- `ECHONEXUS_BUILD_TESTS` is disabled by default. Enable it with `-DECHONEXUS_BUILD_TESTS=ON` when you want to build the test targets manually.
- `ECHONEXUS_AUTO_SETUP_VCPKG` only applies when EchoNexus is configured as the top-level project.

## Project Layout

- [include/](/Users/leo/Documents/Development/Contribute/EchoNexus/include) contains the public headers, including the middleware, request, response, and server interfaces
- [src/](/Users/leo/Documents/Development/Contribute/EchoNexus/src) contains the framework implementation
- [example/](/Users/leo/Documents/Development/Contribute/EchoNexus/example) contains a runnable sample application
- [tests/](/Users/leo/Documents/Development/Contribute/EchoNexus/tests) contains Catch2 coverage plus CMake-level regression checks for build integration
- [benchmarks/](/Users/leo/Documents/Development/Contribute/EchoNexus/benchmarks) contains the cross-framework benchmark harness and benchmark apps
- [cmake/](/Users/leo/Documents/Development/Contribute/EchoNexus/cmake) contains shared CMake helpers, including the local `vcpkg` bootstrap logic
- [CMakeLists.txt](/Users/leo/Documents/Development/Contribute/EchoNexus/CMakeLists.txt) defines the main library build, dependency discovery, and install rules
