import os from "node:os";
import { unlink } from "node:fs/promises";
import { join, resolve } from "node:path";

import { FRAMEWORKS } from "../configs/frameworks";
import { buildCases, WORKLOAD_PATHS } from "./cases";
import { waitForHealth } from "./health";
import { buildOhaCommand, ensureOhaOutputDirectory, parseOhaOutput } from "./load";
import { parseOptions } from "./options";
import { buildFrameworkSkipResults, commandExists, preflightFrameworks } from "./preflight";
import { writeReportFiles } from "./report";
import {
  buildCaseFailureSkipResult,
  buildGlobalCommandSkipResults,
  prepareFrameworksWithSkips,
} from "./runtime";
import { startSampler, summarizeResourceSamples } from "./sampler";
import { collectSystemMetadata } from "./system";
import { collectToolchainMetadata } from "./toolchains";
import type {
  BenchmarkOptions,
  BenchmarkSessionReport,
  CaseResult,
  CommandResult,
  CommandSpec,
  FrameworkId,
  LoadMetrics,
  PlatformKind,
  ResourceMetrics,
} from "./types";

const benchmarksRoot = resolve(import.meta.dir, "..");
const repoRoot = resolve(benchmarksRoot, "..");

function detectPlatform(): PlatformKind {
  if (process.platform === "darwin") {
    return "darwin";
  }

  if (process.platform === "linux") {
    return "linux";
  }

  if (process.platform === "win32") {
    return "windows";
  }

  throw new Error(`Unsupported platform: ${process.platform}`);
}

function sleep(ms: number): Promise<void> {
  return new Promise((resolveTimer) => setTimeout(resolveTimer, ms));
}

async function readPipe(pipe: ReadableStream<Uint8Array> | null): Promise<string> {
  if (pipe === null) {
    return "";
  }

  return new Response(pipe).text();
}

async function runCommand(spec: CommandSpec): Promise<CommandResult> {
  if (!spec.silent) {
    console.log(`\n==> ${spec.description}`);
    console.log(`$ ${spec.cmd.join(" ")}`);
  }

  const env = {
    ...process.env,
    ...spec.env,
  };
  delete env.NO_COLOR;

  const child = Bun.spawn({
    cmd: spec.cmd,
    cwd: spec.cwd,
    env,
    stdout: "pipe",
    stderr: "pipe",
  });
  const stdoutPromise = readPipe(child.stdout);
  const stderrPromise = readPipe(child.stderr);
  const exitCode = await child.exited;
  const [stdout, stderr] = await Promise.all([stdoutPromise, stderrPromise]);

  if (exitCode !== 0) {
    throw Object.assign(new Error(`Command failed (${exitCode}): ${spec.cmd.join(" ")}`), {
      exitCode,
      stdout,
      stderr,
    });
  }

  if (!spec.silent && stdout.trim()) {
    console.log(stdout.trim());
  }

  return { stdout, stderr };
}

async function missingCommands(commands: string[]): Promise<string[]> {
  const missing: string[] = [];

  for (const command of commands) {
    if (!(await commandExists(command))) {
      missing.push(command);
    }
  }

  return missing;
}

function requiredGlobalCommands(needsOha: boolean): string[] {
  if (!needsOha) {
    return [];
  }

  return ["oha"];
}

async function prepareFramework(
  platform: PlatformKind,
  buildProfile: BenchmarkOptions["buildProfile"],
  frameworkId: FrameworkId,
): Promise<void> {
  const framework = FRAMEWORKS[frameworkId];
  const steps = framework.setup({
    repoRoot,
    benchmarksRoot,
    platform,
    buildProfile,
  });

  for (const step of steps) {
    await runCommand(step);
  }
}

async function preflightSelectedFrameworks(
  platform: PlatformKind,
  frameworks: FrameworkId[],
) {
  const result = await preflightFrameworks({
    platform,
    frameworks,
    commandExists,
  });

  for (const skippedFramework of result.skippedFrameworks) {
    console.warn(
      `Skipping ${FRAMEWORKS[skippedFramework.framework].displayName}: ${skippedFramework.notes.join("; ")}`,
    );
  }

  return result;
}

function warnSkippedFrameworks(skippedFrameworks: { framework: FrameworkId; notes: string[] }[]): void {
  for (const skippedFramework of skippedFrameworks) {
    console.warn(`Skipping ${FRAMEWORKS[skippedFramework.framework].displayName}: ${skippedFramework.notes.join("; ")}`);
  }
}

async function listUnixProcessTree(rootPid: number): Promise<number[]> {
  const child = Bun.spawn({
    cmd: ["ps", "-axo", "pid=,ppid="],
    stdout: "pipe",
    stderr: "pipe",
  });
  const stdout = await readPipe(child.stdout);
  const stderr = await readPipe(child.stderr);
  const exitCode = await child.exited;

  if (exitCode !== 0) {
    throw new Error(`Failed to list process tree: ${stderr.trim()}`);
  }

  const rows = stdout
    .split("\n")
    .map((line) => line.trim())
    .filter(Boolean)
    .map((line) => {
      const [pidRaw, ppidRaw] = line.split(/\s+/);
      return {
        pid: Number(pidRaw),
        ppid: Number(ppidRaw),
      };
    });
  const children = new Map<number, number[]>();

  for (const row of rows) {
    const branch = children.get(row.ppid) ?? [];
    branch.push(row.pid);
    children.set(row.ppid, branch);
  }

  const queue = [rootPid];
  const visited = new Set<number>();

  while (queue.length > 0) {
    const pid = queue.shift()!;
    if (visited.has(pid)) {
      continue;
    }

    visited.add(pid);
    for (const childPid of children.get(pid) ?? []) {
      queue.push(childPid);
    }
  }

  return [...visited];
}

async function stopProcessTree(rootPid: number): Promise<void> {
  const platform = detectPlatform();

  if (platform === "windows") {
    const child = Bun.spawn({
      cmd: ["taskkill", "/PID", String(rootPid), "/T", "/F"],
      stdout: "ignore",
      stderr: "ignore",
    });
    await child.exited;
    return;
  }

  let pids: number[];
  try {
    pids = await listUnixProcessTree(rootPid);
  } catch {
    pids = [rootPid];
  }

  for (const signal of ["TERM", "KILL"]) {
    for (const pid of [...pids].reverse()) {
      const child = Bun.spawn({
        cmd: ["kill", `-${signal}`, String(pid)],
        stdout: "ignore",
        stderr: "ignore",
      });
      await child.exited;
    }

    await sleep(300);
  }
}

async function runOhaAndParse(options: {
  url: string;
  concurrency: number;
  durationSeconds: number;
  outputFormat: "json" | "quiet";
  outputPath?: string;
}): Promise<LoadMetrics | null> {
  if (options.outputPath !== undefined) {
    await ensureOhaOutputDirectory(options.outputPath);
  }

  await runCommand({
    description: `Run oha against ${options.url}`,
    cmd: buildOhaCommand(options),
    cwd: benchmarksRoot,
  });

  if (options.outputFormat === "quiet" || options.outputPath === undefined) {
    return null;
  }

  const raw = (await Bun.file(options.outputPath).json()) as unknown;
  await unlink(options.outputPath).catch(() => undefined);
  return parseOhaOutput(raw as Parameters<typeof parseOhaOutput>[0]);
}

interface FailureLike {
  message?: unknown;
  exitCode?: unknown;
  stdout?: unknown;
  stderr?: unknown;
}

function emptyMetrics(): LoadMetrics & ResourceMetrics {
  return {
    requestsPerSec: null,
    successRequestsPerSec: null,
    p50Ms: null,
    p95Ms: null,
    p99Ms: null,
    maxMs: null,
    errorRate: null,
    peakRssMb: null,
    avgCpuPercent: null,
  };
}

function emptyResourceMetrics(): ResourceMetrics {
  return {
    peakRssMb: null,
    avgCpuPercent: null,
  };
}

function baseCaseResult(
  frameworkId: FrameworkId,
  workload: CaseResult["workload"],
  workers: number,
  concurrency: number,
  options: BenchmarkOptions,
  executionModel: CaseResult["executionModel"],
): Omit<CaseResult, keyof LoadMetrics | keyof ResourceMetrics | "notes"> {
  return {
    framework: frameworkId,
    workload,
    workers,
    mode: workers > 1 ? "multi-worker" : "single-worker",
    concurrency,
    executionModel,
    warmupSeconds: options.warmupSeconds,
    measureSeconds: options.measureSeconds,
    status: "passed",
  };
}

async function runBenchmarks(options: BenchmarkOptions): Promise<void> {
  const platform = detectPlatform();
  const startedAt = new Date();
  const frameworkPreflight = await preflightSelectedFrameworks(platform, options.frameworks);
  const results: CaseResult[] = buildFrameworkSkipResults({
    options,
    skippedFrameworks: frameworkPreflight.skippedFrameworks,
  });
  const requiredCommands = requiredGlobalCommands(frameworkPreflight.availableFrameworks.length > 0);
  const missingGlobalCommands = await missingCommands(requiredCommands);

  if (missingGlobalCommands.length > 0) {
    const [system, toolchains] = await Promise.all([
      collectSystemMetadata(platform),
      collectToolchainMetadata(runCommand, benchmarksRoot, repoRoot),
    ]);

    console.warn(`Skipping benchmark session: missing required global command: ${missingGlobalCommands.join(", ")}`);
    results.push(
      ...buildGlobalCommandSkipResults({
        options,
        missingCommands: missingGlobalCommands,
        skippedFrameworks: frameworkPreflight.skippedFrameworks,
      }),
    );

    const finishedAt = new Date();
    const report: BenchmarkSessionReport = {
      generatedAt: finishedAt.toISOString(),
      startedAt: startedAt.toISOString(),
      finishedAt: finishedAt.toISOString(),
      totalDurationSeconds: (finishedAt.getTime() - startedAt.getTime()) / 1000,
      config: {
        frameworks: options.frameworks,
        workloads: options.workloads,
        workers: options.workers,
        concurrency: options.concurrency,
        buildProfile: options.buildProfile,
        warmupSeconds: options.warmupSeconds,
        measureSeconds: options.measureSeconds,
        cooldownSeconds: options.cooldownSeconds,
        outputTag: options.outputTag,
      },
      platform,
      cpuCount:
        typeof os.availableParallelism === "function"
          ? os.availableParallelism()
          : os.cpus().length,
      system,
      toolchains,
      results,
    };
    const files = await writeReportFiles(benchmarksRoot, report);

    console.log(`\nResults written to ${files.jsonPath}`);
    console.log(`Summary written to ${files.markdownPath}`);
    return;
  }

  const frameworkPreparation = await prepareFrameworksWithSkips({
    options,
    frameworks: frameworkPreflight.availableFrameworks,
    prepareFramework: (frameworkId) => prepareFramework(platform, options.buildProfile, frameworkId),
  });
  warnSkippedFrameworks(frameworkPreparation.skippedFrameworks);
  results.push(...frameworkPreparation.skipResults);

  const runnableFrameworks = frameworkPreparation.availableFrameworks;

  const [system, toolchains] = await Promise.all([
    collectSystemMetadata(platform),
    collectToolchainMetadata(runCommand, benchmarksRoot, repoRoot),
  ]);

  const port = Number(process.env.BENCHMARK_PORT ?? "18080");
  const cases =
    runnableFrameworks.length === 0
      ? []
      : buildCases({
          frameworks: runnableFrameworks,
          workloads: options.workloads,
          workers: options.workers,
          concurrency: options.concurrency,
        });

  for (const benchmarkCase of cases) {
    const framework = FRAMEWORKS[benchmarkCase.framework];
    const baseResult = baseCaseResult(
      benchmarkCase.framework,
      benchmarkCase.workload,
      benchmarkCase.workers,
      benchmarkCase.concurrency,
      options,
      framework.executionModel,
    );
    const support = framework.supportsWorkers({
      platform,
      workers: benchmarkCase.workers,
    });

    if (!support.supported) {
      results.push({
        ...baseResult,
        ...emptyMetrics(),
        status: "skipped",
        notes: [support.reason ?? "unsupported worker configuration"],
      });
      continue;
    }

    console.log(
      `\n## ${framework.displayName} workers=${benchmarkCase.workers} ${benchmarkCase.workload} c=${benchmarkCase.concurrency}`,
    );

    let app:
      | ReturnType<typeof Bun.spawn>
      | undefined;
    let stdoutPromise: Promise<string> | undefined;
    let stderrPromise: Promise<string> | undefined;
    let finalResult: CaseResult | null = null;
    let failure: FailureLike | null = null;

    try {
      const launch = framework.launch({
        repoRoot,
        benchmarksRoot,
        platform,
        buildProfile: options.buildProfile,
        port,
        workers: benchmarkCase.workers,
        mode: benchmarkCase.mode,
      });

      app = Bun.spawn({
        cmd: launch.cmd,
        cwd: launch.cwd,
        env: {
          ...process.env,
          ...launch.env,
        },
        stdout: "pipe",
        stderr: "pipe",
      });
      stdoutPromise = readPipe(app.stdout);
      stderrPromise = readPipe(app.stderr);

      await waitForHealth(`http://127.0.0.1:${port}/healthz`);

      const targetUrl = `http://127.0.0.1:${port}${WORKLOAD_PATHS[benchmarkCase.workload]}`;
      await runOhaAndParse({
        url: targetUrl,
        concurrency: benchmarkCase.concurrency,
        durationSeconds: options.warmupSeconds,
        outputFormat: "quiet",
      });

      let sampler: Awaited<ReturnType<typeof startSampler>> | null = null;
      const notes = [...launch.notes];

      try {
        sampler = await startSampler(app.pid);
      } catch (error) {
        notes.push(
          `resource sampling unavailable: ${error instanceof Error ? error.message : String(error)}`,
        );
      }

      const outputPath = join(
        benchmarksRoot,
        "results",
        `raw-${benchmarkCase.framework}-${benchmarkCase.workload}-${benchmarkCase.workers}-${benchmarkCase.concurrency}-${Date.now()}.json`,
      );
      const loadMetrics = await runOhaAndParse({
        url: targetUrl,
        concurrency: benchmarkCase.concurrency,
        durationSeconds: options.measureSeconds,
        outputFormat: "json",
        outputPath,
      });
      if (loadMetrics === null) {
        throw new Error("oha did not produce JSON metrics");
      }

      let resourceMetrics = emptyResourceMetrics();
      if (sampler !== null) {
        try {
          resourceMetrics = summarizeResourceSamples(await sampler.stop());
        } catch (error) {
          notes.push(
            `resource sampling unavailable: ${error instanceof Error ? error.message : String(error)}`,
          );
        }
      }

      finalResult = {
        ...baseResult,
        ...loadMetrics,
        ...resourceMetrics,
        status: "passed",
        notes,
      };
    } catch (error) {
      failure = error as FailureLike;
    } finally {
      let stdout = "";
      let stderr = "";
      let exitCode: number | undefined;
      let exitedBeforeShutdown = false;

      if (app) {
        const earlyExit = await Promise.race([
          app.exited.then((code) => ({ finished: true as const, code })),
          sleep(1).then(() => ({ finished: false as const })),
        ]);

        if (earlyExit.finished) {
          exitedBeforeShutdown = true;
          exitCode = earlyExit.code;
        } else {
          await stopProcessTree(app.pid).catch(() => undefined);
          await app.exited.catch(() => undefined);
        }
      }

      if (stdoutPromise) {
        stdout = await stdoutPromise.catch(() => "");
      }

      if (stderrPromise) {
        stderr = await stderrPromise.catch(() => "");
      }

      if (finalResult !== null) {
        results.push(finalResult);
      } else if (failure !== null) {
        results.push(
          buildCaseFailureSkipResult({
            case: benchmarkCase,
            executionModel: framework.executionModel,
            warmupSeconds: options.warmupSeconds,
            measureSeconds: options.measureSeconds,
            error: failure,
            exitCode:
              typeof failure.exitCode === "number"
                ? failure.exitCode
                : exitedBeforeShutdown && exitCode !== 0
                  ? exitCode
                  : undefined,
            stdout,
            stderr,
          }),
        );
      }

      await sleep(options.cooldownSeconds * 1000);
    }
  }

  const finishedAt = new Date();
  const report: BenchmarkSessionReport = {
    generatedAt: finishedAt.toISOString(),
    startedAt: startedAt.toISOString(),
    finishedAt: finishedAt.toISOString(),
    totalDurationSeconds: (finishedAt.getTime() - startedAt.getTime()) / 1000,
    config: {
      frameworks: options.frameworks,
      workloads: options.workloads,
      workers: options.workers,
      concurrency: options.concurrency,
      buildProfile: options.buildProfile,
      warmupSeconds: options.warmupSeconds,
      measureSeconds: options.measureSeconds,
      cooldownSeconds: options.cooldownSeconds,
      outputTag: options.outputTag,
    },
    platform,
    cpuCount:
      typeof os.availableParallelism === "function"
        ? os.availableParallelism()
        : os.cpus().length,
    system,
    toolchains,
    results,
  };
  const files = await writeReportFiles(benchmarksRoot, report);

  console.log(`\nResults written to ${files.jsonPath}`);
  console.log(`Summary written to ${files.markdownPath}`);
}

async function main(): Promise<void> {
  const platform = detectPlatform();
  const options = parseOptions(Bun.argv.slice(2));

  if (options.setupOnly) {
    const frameworkPreflight = await preflightSelectedFrameworks(platform, options.frameworks);
    const frameworkPreparation = await prepareFrameworksWithSkips({
      options,
      frameworks: frameworkPreflight.availableFrameworks,
      prepareFramework: (frameworkId) => prepareFramework(platform, options.buildProfile, frameworkId),
    });
    warnSkippedFrameworks(frameworkPreparation.skippedFrameworks);
    return;
  }

  await runBenchmarks(options);
}

await main();
