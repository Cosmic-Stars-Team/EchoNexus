set shell := ["sh", "-c"]
set windows-shell := ["pwsh.exe", "-NoProfile", "-Command"]

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
configure compiler="" release="" debug_flag="":
    @: {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ if compiler != "" { "cmake -S . -B build/" + replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" + (if release != "" { "release" } else { "debug" }) + " -G Ninja -DCMAKE_BUILD_TYPE=" + (if release != "" { "Release" } else { "Debug" }) + " \"-DCMAKE_CXX_COMPILER=" + compiler + "\"" } else if release != "" { "cmake --preset release" } else { "cmake --preset debug" } }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
build compiler="" release="" debug_flag="": (configure compiler release debug_flag)
    @: {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ if compiler != "" { "cmake --build build/" + replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" + (if release != "" { "release" } else { "debug" }) } else if release != "" { "cmake --build --preset build-release" } else { "cmake --build --preset build-debug" } }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("export_path", long="export", help="Write JUnit XML test results to the given path")]
[arg("release", long="release", value="release", help="Use the release profile")]
test label="" compiler="" export_path="" release="" debug_flag="": (build compiler release debug_flag)
    @: {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ if compiler != "" { "ctest --test-dir build/" + replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" + (if release != "" { "release" } else { "debug" }) + " --output-on-failure" + (if label != "" { " -L " + label } else { "" }) + (if export_path != "" { " --output-junit \"" + export_path + "\"" } else { "" }) } else if release != "" { "ctest --test-dir build/release --output-on-failure" + (if label != "" { " -L " + label } else { "" }) + (if export_path != "" { " --output-junit \"" + export_path + "\"" } else { "" }) } else { "ctest --test-dir build/debug --output-on-failure" + (if label != "" { " -L " + label } else { "" }) + (if export_path != "" { " --output-junit \"" + export_path + "\"" } else { "" }) } }}

[arg("compiler", long="compiler", help="C++ compiler command or path")]
[arg("debug_flag", long="debug", value="debug", help="Use the debug profile (default)")]
[arg("release", long="release", value="release", help="Use the release profile")]
run compiler="" release="" debug_flag="": (configure compiler release debug_flag)
    @: {{ if release != "" { if debug_flag != "" { error("`--debug` and `--release` are mutually exclusive") } else { "" } } else { "" } }}
    {{ if compiler != "" { "cmake --build build/" + replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" + (if release != "" { "release" } else { "debug" }) + " --target example" } else if release != "" { "cmake --build --preset build-release --target example" } else { "cmake --build --preset build-debug --target example" } }}
    {{ if compiler != "" { "cmake -E chdir build/" + replace(replace(replace(replace(replace(compiler, "/", "_"), "\\", "_"), " ", "_"), "+", "x"), ":", "_") + "-" + (if release != "" { "release" } else { "debug" }) + " example/example" + (if os_family() == "windows" { ".exe" } else { "" }) } else if release != "" { "cmake -E chdir build/release example/example" + (if os_family() == "windows" { ".exe" } else { "" }) } else { "cmake -E chdir build/debug example/example" + (if os_family() == "windows" { ".exe" } else { "" }) } }}
