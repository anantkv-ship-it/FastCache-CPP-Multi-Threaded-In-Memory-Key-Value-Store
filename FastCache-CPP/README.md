# FastCache-CPP 

A C++17 multi-threaded in-memory key-value store exposed through a small TCP protocol. It demonstrates:

- Thread-safe concurrent access
- `std::unordered_map` + custom doubly-linked list for O(1) LRU operations
- Configurable worker thread pool
- TCP client/server communication
- TTL expiration using a min-heap
- Asynchronous Append-Only File (AOF) persistence
- GoogleTest unit tests
- Dockerized build/runtime

> **Performance numbers are intentionally not hard-coded into this README.**

## Architecture

```text
             TCP Clients
                  |
                  v
        +---------------------+
        |    TCP Listener     |
        +----------+----------+
                   |
             accepted sockets
                   |
                   v
        +---------------------+
        |    Worker Pool      |
        |  N worker threads   |
        +----------+----------+
                   |
                   v
        +---------------------+
        |  Command Processor  |
        +----------+----------+
                   |
                   v
        +--------------------------------------+
        |          Thread-Safe Store           |
        |                                      |
        | unordered_map<Key, Node*>            |
        |          +                           |
        | custom doubly-linked LRU list        |
        |          +                           |
        | shared_mutex                         |
        +-----------+--------------------------+
                    |
          +---------+---------+
          |                   |
          v                   v
   TTL Expiration         AOF Logger
    background            background
      thread                 thread
```

## Repository Layout

```text
FastCache-CPP/
├── CMakeLists.txt
├── Dockerfile
├── LICENSE
├── README.md
├── include/
│   ├── aof_logger.hpp
│   ├── config.hpp
│   ├── kv_store.hpp
│   ├── lru_cache.hpp
│   ├── protocol.hpp
│   ├── server.hpp
│   ├── thread_pool.hpp
│   └── ttl_manager.hpp
├── src/
│   ├── aof_logger.cpp
│   ├── config.cpp
│   ├── kv_store.cpp
│   ├── lru_cache.cpp
│   ├── main.cpp
│   ├── protocol.cpp
│   ├── server.cpp
│   ├── thread_pool.cpp
│   └── ttl_manager.cpp
├── tests/
│   ├── test_kv_store.cpp
│   ├── test_lru.cpp
│   └── test_protocol.cpp
├── benchmarks/
│   └── load_test.py
├── examples/
│   └── client.sh
└── docker/
    └── docker-compose.yml
```

## Supported Commands

Each request is one newline-terminated command.

```text
SET key value
SET key value EX seconds
GET key
DEL key
EXPIRE key seconds
PING
STATS
QUIT
```

Examples:

```text
SET user:1001 Anant
OK

GET user:1001
Anant

SET session:token active EX 60
OK

EXPIRE session:token 120
OK

DEL user:1001
1

PING
PONG
```

Values containing spaces should be quoted:

```text
SET greeting "hello world"
```

## Build

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Tests:

```bash
ctest --test-dir build --output-on-failure
```

Run:

```bash
./build/fastcache_server --port 8080 --threads 4 --max-items 100000 --aof data/appendonly.aof
```

### Docker

```bash
docker build -t fastcache-cpp .
docker run --rm -p 8080:8080 -v "$(pwd)/data:/data" fastcache-cpp
```

Or:

```bash
docker compose -f docker/docker-compose.yml up --build
```

## Example Client

With the server running:

```bash
./examples/client.sh
```

Or use netcat:

```bash
nc 127.0.0.1 8080
```

## LRU Design

The cache maintains two structures:

```text
unordered_map
    key ───────────────► Node*

LRU list:
MRU                                  LRU
  |                                    |
  v                                    v
[ A ] <-> [ C ] <-> [ D ] <-> [ B ]
```

The hash map gives average O(1) key lookup. The doubly-linked list allows:

- Move an accessed key to MRU: O(1)
- Insert at MRU: O(1)
- Remove a node: O(1)
- Evict LRU: O(1)

The store owns the synchronization around these structures so the map and
list cannot become inconsistent.

## Concurrency Model

The TCP listener accepts connections and places sockets into a bounded
worker queue. Worker threads process client requests.

The store uses `std::shared_mutex`:

- Read-only map lookup can use a shared lock.
- Structural mutations use an exclusive lock.
- LRU recency updates are exclusive because they modify the linked list.

A GET therefore performs a safe lookup followed by a short exclusive LRU
touch. This preserves correctness while allowing the expensive lookup phase
to use shared access.

## TTL Design

TTL metadata is maintained in a min-heap ordered by expiration time.

```text
      earliest expiry
             |
             v
        +---------+
        | min heap|
        +---------+
             |
       expiration
        thread
             |
             v
        DEL / expire
```

A background thread waits until the nearest deadline and removes expired
entries. Stale heap records are ignored using a generation/version value, so
an old expiration cannot delete a key after that key has received a newer TTL.

## AOF Persistence

Write commands are placed into an in-memory queue and consumed by a dedicated
logger thread.

```text
SET / DEL / EXPIRE
        |
        v
   AOF queue
        |
        v
  logger thread
        |
        v
appendonly.aof
```

On startup, the server replays valid AOF commands before accepting clients.

The AOF is intentionally simple and educational rather than a production-grade
database WAL. A flush occurs after each command by default for easier crash
recovery demonstrations.

## Benchmarking

Start the server:

```bash
./build/fastcache_server --port 8080 --threads 8 --max-items 100000
```

Run:

```bash
python3 benchmarks/load_test.py --host 127.0.0.1 --port 8080 \
    --clients 50 --requests 1000
```

The benchmark prints total operations, elapsed time, throughput and average
latency. Benchmark results depend heavily on CPU, OS, compiler, network stack,
payload size, persistence configuration and machine load.

## Design Trade-offs

### Why a worker pool?

Creating an unbounded thread for every connection can create excessive
scheduling and memory overhead. A bounded worker pool provides predictable
resource usage.

### Why not `std::list`?

The project intentionally uses a custom doubly-linked list to demonstrate
pointer ownership, O(1) node manipulation and explicit cache invariants.

### Why `shared_mutex`?

It communicates the intended read/write synchronization policy and allows
multiple threads to perform the read-only portion of a lookup concurrently.

### Why asynchronous AOF?

Disk I/O should not hold up the request-processing thread. Commands are handed
to the logger queue, allowing the network worker to continue.




