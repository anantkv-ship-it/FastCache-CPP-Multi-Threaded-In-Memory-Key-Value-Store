#!/usr/bin/env python3
"""Simple concurrent TCP benchmark for FastCache-CPP."""

import argparse
import socket
import statistics
import time
from concurrent.futures import ThreadPoolExecutor


def worker(host, port, requests):
    latencies = []
    operations = 0

    with socket.create_connection((host, port), timeout=5) as sock:
        file = sock.makefile("rwb", buffering=0)

        for i in range(requests):
            key = f"bench:{i % 1000}"
            command = f"SET {key} {i}\n".encode()

            start = time.perf_counter()
            file.write(command)
            response = file.readline()
            elapsed = (time.perf_counter() - start) * 1000

            if not response:
                raise RuntimeError("server closed connection")

            latencies.append(elapsed)
            operations += 1

    return operations, latencies


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8080)
    parser.add_argument("--clients", type=int, default=20)
    parser.add_argument("--requests", type=int, default=1000)
    args = parser.parse_args()

    start = time.perf_counter()

    with ThreadPoolExecutor(max_workers=args.clients) as pool:
        futures = [
            pool.submit(worker, args.host, args.port, args.requests)
            for _ in range(args.clients)
        ]
        results = [f.result() for f in futures]

    elapsed = time.perf_counter() - start
    latencies = [x for _, ls in results for x in ls]
    operations = sum(x for x, _ in results)

    print(f"operations:        {operations}")
    print(f"elapsed:            {elapsed:.3f} s")
    print(f"throughput:         {operations / elapsed:,.0f} ops/sec")
    print(f"avg latency:        {statistics.mean(latencies):.3f} ms")
    print(f"p95 latency:        {statistics.quantiles(latencies, n=20)[18]:.3f} ms")


if __name__ == "__main__":
    main()
