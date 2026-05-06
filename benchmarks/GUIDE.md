# Benchmark Guide

## Scope

This suite compares:

- EchoNexus
- FastAPI
- Flask
- Koa
- Elysia
- Axum
- Gin
- Spring Boot

Traffic stays on `127.0.0.1`. One target runs at a time. The Bun runner writes both JSON and Markdown reports under `benchmarks/results/`.

## CLI

The runner is flag-driven:

```bash
cd benchmarks
bun run src/cli.ts [flags]
```

Supported flags:

- `--frameworks echonexus,fastapi,flask,koa,elysia,axum,gin,springboot`
- `--workloads plaintext,json,time_json,param_json_middleware`
- `--workers 1,4`
- `--concurrency 50,200,500,1000,2000,5000`
- `--release`
- `--debug`
- `--warmup 10`
- `--measure 30`
- `--cooldown 5`
- `--output-tag my-run`
- `--setup`

Rules:

- `--frameworks`, `--workloads`, `--workers`, and `--concurrency` accept comma-separated lists.
- `--workers` accepts any positive integer list.
- `--concurrency` accepts any positive integer list. `4999` is valid.
- `--warmup` and `--cooldown` accept `0` or higher.
- `--measure` must be at least `1`.
- `--release` and `--debug` are mutually exclusive.
- With no flags, the runner executes the full default matrix.

Default matrix:

- frameworks: all
- workloads: all
- workers: `1,4`
- concurrency: `50,200,500,1000,2000,5000`
- build profile: `release`
- warmup: `10`
- measure: `30`
- cooldown: `5`

## Wrapper Scripts

These are convenience wrappers only:

- `bun run setup`
- `bun run smoke`
- `bun run pilot`
- `bun run full`

`full` is just `bun run src/cli.ts`. The preset names are not special runner subcommands anymore.

## Quick Start

```bash
cd benchmarks
bun run src/cli.ts --frameworks echonexus,axum --workloads plaintext,time_json --workers 1,4 --concurrency 50,200,500 --warmup 5 --measure 10 --cooldown 2 --release
```

Generated output:

- `benchmarks/results/<timestamp>.json`
- `benchmarks/results/<timestamp>.md`

If `--output-tag nightly` is provided, filenames become `...--nightly.json` and `...--nightly.md`.

## Workloads

- `plaintext`: the lightest text response path
- `json`: serialize a native object to JSON using each framework's JSON support (no hand-crafted response strings)
- `time_json`: per-request local time generation plus JSON serialization
- `param_json_middleware`: path params, middleware dispatch, request-local state, and JSON serialization together

Routes exposed by every benchmark app:

- `GET /healthz`
- `GET /plaintext`
- `GET /json`
- `GET /time_json`
- `GET /api/v1/users/:id/profile`

## Fairness Rules

- HTTP/1.1 only
- no TLS
- no compression
- no access logs
- no database or external network I/O
- no filesystem work inside handlers

Middleware alignment rule:

- every framework runs a global middleware or filter on every request
- this middleware sets a `scope=user` attribute when the path starts with `/api/v1/users/`, and does nothing otherwise
- all requests go through this middleware — even those that don't match — so the dispatch overhead is always included

Unsupported worker combinations are marked `skipped`. They are never silently run with a different worker count.

Example: Elysia `workers=4` on macOS is skipped in this suite because Bun shared-port multi-process mode is Linux-only here.

## Report Fields

The JSON file is the source of truth. The Markdown file is designed to be readable on its own.

Important fields:

- `Status`: `passed`, `skipped`, or `failed`
- `QPS`: raw request throughput
- `Success QPS`: `QPS * (1 - errorRate)`, computed when `oha` reports both total requests and error count
- `Err %`: request error ratio
- `Execution Model`: descriptive runtime architecture, not a fairness claim

Each session also records:

- generated/start/finish timestamps
- total session duration
- resolved frameworks/workloads/workers/concurrency
- selected build profile
- warmup/measure/cooldown seconds
- OS, kernel, CPU, memory, hostname
- runtime and compiler versions actually observed on the host

If resource sampling is unavailable on the current host, RSS (resident set size) and CPU fields become `-` and the reason is written into `Notes`.

## Required Toolchains

- Bun
- `oha`
- CMake
- a C/C++ compiler toolchain for EchoNexus
- Python 3
- Node.js
- Rust toolchain
- Go
- Java

Suggested `oha` installation:

```bash
cargo install oha
```

Spring Boot notes:

- the runner uses `./gradlew` automatically when it exists
- if no wrapper is present, it falls back to `gradle`

Go notes:

- on macOS, the runner also checks common Homebrew Go install paths before failing

## Setup

The benchmark runner prepares the selected frameworks automatically before load testing. `bun run setup` is still available when you want to prepare/build everything without running `oha`.

Current setup behavior:

- EchoNexus: local CMake preset entrypoint from `benchmarks/apps/echonexus` via `benchmarks/apps/echonexus/CMakePresets.json`
- FastAPI: create `.venv` and install requirements
- Flask: create `.venv` and install requirements
- Koa: `bun install --cache-dir ../../.bun-cache --no-progress`
- Elysia: `bun install --cache-dir ../../.bun-cache --no-progress`
- Axum: `cargo build --release` or `cargo build`
- Gin: `go mod download` + `go build -o build/gin-benchmark .`
- Spring Boot: Gradle `bootJar`

`setup` expects toolchains to already be installed. It does not install Bun, Python, Node, Rust, Go, Java, or `oha` for you.

For EchoNexus, the benchmark runner uses the same local preset entrypoint and preset file: `benchmarks/apps/echonexus/CMakePresets.json`. Manual setup is:

```bash
cd benchmarks/apps/echonexus
cmake --preset release
cmake --build --preset build-release --target echonexus_benchmark
```

For debug builds, use the corresponding `debug` and `build-debug` presets.

At startup, the runner preflights the selected frameworks. If a framework-specific SDK or command is missing, that framework is marked `skipped` immediately and the runner avoids spending setup time on it.

## Platform Notes

- macOS, Linux, and Windows are supported by the runner
- Elysia multi-worker shared-port mode is only benchmarked on Linux
- process-tree RSS/CPU sampling uses `ps` on macOS/Linux and PowerShell/WMI on Windows
- sandboxed environments may block process sampling; the benchmark still runs, but RSS/CPU metrics can be unavailable
