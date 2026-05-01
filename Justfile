set shell := ["sh", "-c"]
set windows-shell := ["pwsh.exe", "-NoProfile", "-Command"]

clean:
    cmake -E rm -rf build CMakeFiles CMakeCache.txt cmake_install.cmake Testing compile_commands.json

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
configure compiler="" release="" debug_flag="":
    {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ "cmake -D ECHONEXUS_SOURCE_DIR=. -D ECHONEXUS_BINARY_DIR=build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release" } else { "debug" }) + " -D ECHONEXUS_BUILD_TYPE=" + (if release != "" { "Release" } else { "Debug" }) + (if compiler != "" { " " + (if os_family() == "windows" { "'" + replace("-DECHONEXUS_COMPILER=" + compiler, "'", "''") + "'" } else { quote("-DECHONEXUS_COMPILER=" + compiler) }) } else { "" }) + " -P cmake/ConfigureWithCompilerEnv.cmake" }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
build compiler="" release="" debug_flag="": (configure compiler release debug_flag)
    {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ "cmake --build build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release" } else { "debug" }) }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
[arg("export_path", long="export", help="Write JUnit XML test results to the given path")]
test label="" compiler="" export_path="" release="" debug_flag="":
    {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ "cmake -D ECHONEXUS_SOURCE_DIR=. -D ECHONEXUS_BINARY_DIR=build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release-tests" } else { "debug-tests" }) + " -D ECHONEXUS_BUILD_TYPE=" + (if release != "" { "Release" } else { "Debug" }) + " -D ECHONEXUS_BUILD_TESTS=ON -D ECHONEXUS_BUILD_EXAMPLES=OFF" + (if compiler != "" { " " + (if os_family() == "windows" { "'" + replace("-DECHONEXUS_COMPILER=" + compiler, "'", "''") + "'" } else { quote("-DECHONEXUS_COMPILER=" + compiler) }) } else { "" }) + " -P cmake/ConfigureWithCompilerEnv.cmake" }}
    {{ "cmake --build build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release-tests" } else { "debug-tests" }) }}
    {{ "ctest --test-dir build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release-tests" } else { "debug-tests" }) + " --output-on-failure" + (if label != "" { " -L " + label } else { "" }) + (if export_path != "" { " --output-junit \"" + export_path + "\"" } else { "" }) }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
run compiler="" release="" debug_flag="": (configure compiler release debug_flag)
    {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ "cmake --build build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release" } else { "debug" }) + " --target example" }}
    {{ "cmake -E chdir build/" + (if compiler != "" { replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" } else { "" }) + (if release != "" { "release" } else { "debug" }) + " example/example" + (if os_family() == "windows" { ".exe" } else { "" }) }}

[arg("frameworks", long, help="Comma-separated framework ids")]
[arg("workloads", long, help="Comma-separated workload ids")]
[arg("workers", long, help="Comma-separated worker counts")]
[arg("concurrency", long, help="Comma-separated concurrency values")]
[arg("warmup", long, help="Warmup seconds")]
[arg("measure", long, help="Measure seconds")]
[arg("cooldown", long, help="Cooldown seconds")]
[arg("output_tag", long="output-tag", help="Optional report filename tag")]
[arg("debug_flag", long="debug", value="debug", help="Build compiled targets in debug mode")]
[arg("release", long="release", value="release", help="Build compiled targets in release mode (default)")]
benchmark preset="" frameworks="" workloads="" workers="" concurrency="" warmup="" measure="" cooldown="" output_tag="" debug_flag="" release="":
    {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    cd benchmarks && {{ if preset == "setup" { "bun run setup" + (if debug_flag != "" { " --debug" } else if release != "" { " --release" } else { "" }) } else if preset == "smoke" { "bun run smoke" + (if debug_flag != "" { " --debug" } else if release != "" { " --release" } else { "" }) } else if preset == "pilot" { "bun run pilot" + (if debug_flag != "" { " --debug" } else if release != "" { " --release" } else { "" }) } else if preset == "full" { "bun run full" + (if debug_flag != "" { " --debug" } else if release != "" { " --release" } else { "" }) } else { "bun run src/cli.ts" + (if frameworks != "" { " --frameworks '" + frameworks + "'" } else { "" }) + (if workloads != "" { " --workloads '" + workloads + "'" } else { "" }) + (if workers != "" { " --workers '" + workers + "'" } else { "" }) + (if concurrency != "" { " --concurrency '" + concurrency + "'" } else { "" }) + (if debug_flag != "" { " --debug" } else if release != "" { " --release" } else { "" }) + (if warmup != "" { " --warmup '" + warmup + "'" } else { "" }) + (if measure != "" { " --measure '" + measure + "'" } else { "" }) + (if cooldown != "" { " --cooldown '" + cooldown + "'" } else { "" }) + (if output_tag != "" { " --output-tag '" + output_tag + "'" } else { "" }) } }}
