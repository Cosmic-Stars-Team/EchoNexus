# Tests Guide

This directory contains EchoNexus test code and shared test infrastructure. The layout should stay stable as the test suite grows, so new tests remain easy to find and maintain.

## Directory Layout

- `unit/`
  Stores unit tests. The directory layout should mirror the public module paths in `include/` and stay aligned with the logical module boundaries implemented in `src/`.
- `integration/`
  Stores integration tests. These tests focus on multi-module behavior, such as starting a server in-process, binding a local port, and issuing real HTTP requests against `127.0.0.1`.
- `common/`
  Stores shared test infrastructure such as helpers, fakes, mock executors, request builders, port helpers, and async test utilities.

## Naming

- Test source files should use the `*.test.cpp` suffix.
- Filenames should stay aligned with the module under test whenever possible.
  Example: `include/middlewares/cors.hpp` maps to `tests/unit/middlewares/cors.test.cpp`.

## Formatting

- All new or modified C++ test code must be formatted with the repository root `.clang-format`.
- Do not commit test files that visibly drift from the existing code style.
- Formatting is still required even when a change only touches test code.

## Unit Test Rules

- Unit tests should focus on local behavior in requests, responses, routers, middlewares, and handler chains.
- Unit tests should not start a real server or depend on real network traffic by default.
- When transport behavior needs isolation, prefer fakes or a mock executor from `common/`.
- Unit tests must continue to build and run with `ECHONEXUS_DISABLE_BEAST=ON`.

## Integration Test Rules

- Integration tests validate real collaboration across multiple modules rather than isolated local logic.
- The preferred integration style is to start the server in-process, bind a local port, and assert behavior through real requests against `127.0.0.1`.
- Tests that depend on Boost.Beast transport must respect the `ECHONEXUS_DISABLE_BEAST` switch and must not be mixed into the always-available unit suite.

## `common/` Boundaries

- `common/` is only for reusable test infrastructure.
- Do not move scenario-specific assertions, business logic checks, or the main body of a single test case into `common/`.
- If a helper is only useful to one test file, keep it in that file instead of extracting it early.

## Guidelines For New Tests

- Decide first whether a test is a unit test or an integration test, then choose its location.
- Prefer small, readable tests with one clear responsibility instead of one giant test that tries to cover five scenarios.
- Extract helpers only after reuse is real; do not abstract early based on hypothetical future reuse.
