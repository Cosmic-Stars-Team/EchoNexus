import { describe, expect, test } from "bun:test";
import { mkdtemp, stat } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";

import * as load from "../src/load";

describe("parseOhaOutput", () => {
  test("normalizes oha json summary into benchmark metrics", () => {
    const metrics = load.parseOhaOutput({
      summary: {
        successRate: 0.9875,
        total: 30.4,
        slowest: 0.011,
        fastest: 0.0008,
        average: 0.0021,
        requestsPerSec: 4321.25,
        totalData: 123456,
        sizePerRequest: 32,
        sizePerSec: 4096,
      },
      responseTimeHistogram: {},
      latencyPercentiles: {
        p10: 0.001,
        p25: 0.0014,
        p50: 0.0019,
        p75: 0.0028,
        p90: 0.0037,
        p95: 0.0049,
        p99: 0.0072,
        "p99.9": 0.0098,
        "p99.99": 0.011,
      },
      firstByteHistogram: {},
      firstBytePercentiles: {
        p10: 0.0007,
        p25: 0.0008,
        p50: 0.0011,
        p75: 0.0015,
        p90: 0.002,
        p95: 0.0024,
        p99: 0.0031,
        "p99.9": 0.004,
        "p99.99": 0.0051,
      },
      rps: {
        mean: 4300,
        stddev: 10,
        max: 4400,
        min: 4200,
        percentiles: {
          p10: 4201,
          p25: 4250,
          p50: 4300,
          p75: 4350,
          p90: 4380,
          p95: 4390,
          p99: 4399,
          "p99.9": 4400,
          "p99.99": 4400,
        },
      },
      details: {
        DNSDialup: { average: 0, fastest: 0, slowest: 0 },
        DNSLookup: { average: 0, fastest: 0, slowest: 0 },
        firstByte: { average: 0.001, fastest: 0.0009, slowest: 0.003 },
      },
      statusCodeDistribution: {
        "200": 3950,
        "500": 50,
      },
      errorDistribution: {
        "connection reset": 3,
      },
    });

    expect(metrics.requestsPerSec).toBe(4321.25);
    expect(metrics.p50Ms).toBe(1.9);
    expect(metrics.p95Ms).toBe(4.9);
    expect(metrics.p99Ms).toBe(7.2);
    expect(metrics.maxMs).toBe(11);
    expect(metrics.successRequestsPerSec).toBe(4267.234375);
    expect(metrics.errorRate).toBe(0.0125);
  });

  test("derives success throughput from success rate when requests per second are present", () => {
    const metrics = load.parseOhaOutput({
      summary: {
        requestsPerSec: 1234.5,
        successRate: 0.8,
      },
    });

    expect(metrics.requestsPerSec).toBe(1234.5);
    expect(metrics.successRequestsPerSec).toBe(987.6);
    expect(metrics.errorRate).toBe(0.2);
  });

  test("preserves missing oha metrics as null instead of fake zeroes", () => {
    const metrics = load.parseOhaOutput({
      summary: {
        total: 12.3,
      },
      details: {
        firstByte: { average: 0.001 },
      },
    });

    expect(metrics.requestsPerSec).toBeNull();
    expect(metrics.successRequestsPerSec).toBeNull();
    expect(metrics.p50Ms).toBeNull();
    expect(metrics.p95Ms).toBeNull();
    expect(metrics.p99Ms).toBeNull();
    expect(metrics.maxMs).toBeNull();
    expect(metrics.errorRate).toBeNull();
  });

  test("preserves explicit null values from oha json instead of deriving fake numbers", () => {
    const metrics = load.parseOhaOutput({
      summary: {
        requestsPerSec: 4321.25,
        successRate: null,
        slowest: null,
      },
      latencyPercentiles: {
        p50: null,
        p95: null,
        p99: null,
      },
    });

    expect(metrics.requestsPerSec).toBe(4321.25);
    expect(metrics.successRequestsPerSec).toBeNull();
    expect(metrics.p50Ms).toBeNull();
    expect(metrics.p95Ms).toBeNull();
    expect(metrics.p99Ms).toBeNull();
    expect(metrics.maxMs).toBeNull();
    expect(metrics.errorRate).toBeNull();
  });
});

describe("ensureOhaOutputDirectory", () => {
  test("creates the parent directory for oha json output files", async () => {
    const root = await mkdtemp(join(tmpdir(), "oha-output-"));
    const outputPath = join(root, "results", "raw-case.json");

    expect(typeof (load as { ensureOhaOutputDirectory?: unknown }).ensureOhaOutputDirectory).toBe(
      "function",
    );

    await (
      load as {
        ensureOhaOutputDirectory: (path: string) => Promise<void>;
      }
    ).ensureOhaOutputDirectory(outputPath);

    const info = await stat(join(root, "results"));
    expect(info.isDirectory()).toBe(true);
  });
});
