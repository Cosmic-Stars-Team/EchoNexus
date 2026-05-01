import { mkdir } from "node:fs/promises";
import { dirname } from "node:path";

import type { LoadMetrics } from "./types";

interface OhaLikeSummary {
  requestsPerSec?: number | null;
  successRate?: number | null;
  slowest?: number | null;
  [key: string]: unknown;
}

interface OhaLikeLatencyPercentiles {
  p50?: number | null;
  p95?: number | null;
  p99?: number | null;
  [key: string]: unknown;
}

interface OhaLikePayload {
  summary?: OhaLikeSummary;
  latencyPercentiles?: OhaLikeLatencyPercentiles;
  [key: string]: unknown;
}

export interface OhaCommandOptions {
  url: string;
  concurrency: number;
  durationSeconds: number;
  outputFormat: "json" | "quiet";
  outputPath?: string;
}

export function buildOhaCommand(options: OhaCommandOptions): string[] {
  const command = [
    "oha",
    "--no-tui",
    "--wait-ongoing-requests-after-deadline",
    "-H",
    "Accept-Encoding: identity",
    "-c",
    String(options.concurrency),
    "-z",
    `${options.durationSeconds}s`,
    "--output-format",
    options.outputFormat,
  ];

  if (options.outputPath) {
    command.push("--output", options.outputPath);
  }

  command.push(options.url);
  return command;
}

export async function ensureOhaOutputDirectory(outputPath: string): Promise<void> {
  await mkdir(dirname(outputPath), { recursive: true });
}

function toNullableNumber(value: number | null | undefined): number | null {
  if (value == null) {
    return null;
  }

  return value;
}

function toMilliseconds(seconds: number | null | undefined): number | null {
  if (seconds == null) {
    return null;
  }

  return Number((seconds * 1000).toFixed(4));
}

export function parseOhaOutput(payload: OhaLikePayload): LoadMetrics {
  const summary = payload.summary ?? {};
  const latencyPercentiles = payload.latencyPercentiles ?? {};
  const requestsPerSec = toNullableNumber(summary.requestsPerSec);
  const successRate = toNullableNumber(summary.successRate);

  return {
    requestsPerSec,
    successRequestsPerSec:
      successRate === null || requestsPerSec === null ? null : requestsPerSec * successRate,
    p50Ms: toMilliseconds(latencyPercentiles.p50),
    p95Ms: toMilliseconds(latencyPercentiles.p95),
    p99Ms: toMilliseconds(latencyPercentiles.p99),
    maxMs: toMilliseconds(summary.slowest),
    errorRate: successRate === null ? null : Number((1 - successRate).toFixed(6)),
  };
}
