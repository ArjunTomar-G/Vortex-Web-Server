<div align="center">

# Vortex Engine

**High-Performance Asynchronous HTTP Server in C++17**

Featuring Linux Edge-Triggered `epoll`, a Decoupled Worker Pool, and Zero-Copy Static File Transmission.

[![C++17](https://img.shields.io/badge/C++-17-00599C?logo=c%2B%2B)](https://en.cppreference.com/w/cpp/17)
[![POSIX](https://img.shields.io/badge/API-POSIX_Sockets-FCC624?logo=linux)](https://man7.org/linux/man-pages/man7/socket.7.html)
[![epoll](https://img.shields.io/badge/Multiplexing-Linux_epoll-FCC624?logo=linux)](https://man7.org/linux/man-pages/man7/epoll.7.html)
[![Concurrency](https://img.shields.io/badge/Threading-std::thread-4EAA25)](https://en.cppreference.com/w/cpp/thread)

</div>

---

## Overview

Vortex Engine is an **asynchronous HTTP/1.1 server** built from scratch in C++17 to demonstrate low-level systems programming, non-blocking network I/O, and concurrent architecture. 

Unlike standard synchronous servers that spawn a new OS thread per connection—which quickly exhausts system memory and triggers massive context-switching overhead—Vortex draws inspiration from Nginx's event-driven architecture. It leverages Linux's `epoll` API to multiplex thousands of non-blocking sockets on a single event-loop thread, dispatching ready connections to a synchronized worker pool.

To maximize throughput, Vortex serves static files using Linux's `sendfile()` system call, avoiding an additional copy of the file payload through user-space buffers. The kernel transfers file data directly from the page cache to the socket, eliminating an unnecessary user-space copy. On supported systems, the kernel and network hardware may further optimize this transfer using DMA.

---

## Highlights

- Event-driven architecture using Linux `epoll`
- Edge-triggered (`EPOLLET`) non-blocking I/O
- Fixed-size worker thread pool
- Zero-copy static file serving with `sendfile()`
- Thread-safe logging
- ApacheBench benchmarking

---

### Working (Big Picture)
```text

┌─────────────────┐                                      ┌────────────────────┐
│ Clients (Web)   │                                      │ Linux Page Cache   │
│ 1,000+ Conns    │                                      │ (Static Assets)    │
└────────┬────────┘                                      └─────────┬──────────┘
  ▲      │ 1. Socket trigger (EPOLLIN)                             │ 
  │      ▼                                                         │
  │ ┌─────────────────┐   2. Push FD via Lambda  ┌───────────────┐ │ 4. sendfile() instruction
  │ │ epoll_wait()    │─────────────────────────▶│ Task Queue    │ │
  │ │ (Main Thread)   │                          │ (FIFO)        │ │
  │ └─────────────────┘                          └───────┬───────┘ │
  │          ▲                                           │         │
  │          │                       3. CV notify_one()  │         │
  │          │                                           ▼         ▼
  │          │                             ┌──────────────────────────────┐
  │          │ 6. Yield &                  │ ThreadPool (Worker Threads)  │
  │          │    Wait for next            │ ├── [ Thread 1 (Parsing) ]   │
  │          │    Event                    │ ├── [ Thread 2 (Sleeping)]   │
  │          └─────────────────────────────│ └── [ Thread N ...       ]   │
  │                                        └─────────────┬────────────────┘
  │                                                      │
  │        5. Kernel sendfile() transfers file           │
  │        from Kernel to Network Socket Buffer          │
  └──────────────────────────────────────────────────────┘

```
The system operates in three highly decoupled phases:
1. **Phase 1 (Multiplexing & Event Loop)** — The main thread runs an infinite `epoll_wait` loop. It monitors the server socket for new connections and existing client sockets for incoming data. Because it uses Edge-Triggered (`EPOLLET`) mode, the kernel only notifies the application once when state changes, allowing the event loop to scale with the number of ready file descriptors rather than the total number of open connections.
2. **Phase 2 (Worker Pool Dispatch)** — When a socket is ready to be read, the main thread does *not* read it. Instead, it wraps the socket descriptor in a lambda and pushes it to a synchronized FIFO Task Queue. A sleeping worker thread in the `ThreadPool` is awakened via a `std::condition_variable` to process the request.
3. **Phase 3 (Request Processing & File Transfer)** — The worker thread reads the raw bytes, parses the HTTP request, and determines the target file. Instead of copying the file payload into a user-space buffer, the worker calls `sendfile()`, instructing the OS kernel to transfer the file directly to the socket buffer.

### The Request Lifecycle

```text
      [ Client Connection ]
                │
                ▼
      ┌───────────────────┐
      │ accept() & epoll  │ (Main Thread)
      │ Set O_NONBLOCK    │
      │ Add EPOLLET flag  │
      └─────────┬─────────┘
                │
                ▼
      ┌───────────────────┐
      │ EPOLLIN Triggered │ (Main Thread)
      │ Task Enqueued     │
      └─────────┬─────────┘
                │
                ▼
      ┌───────────────────┐
      │ Worker Thread     │ (Thread Pool)
      │ Dequeues Task     │
      └─────────┬─────────┘
                │
                ▼
        ┌───────────────┐
  ┌────▶│ read() loop   │ (Edge-Triggered: must read
  │     └──────┬────────┘  until EAGAIN/EWOULDBLOCK)
  │            │
  └────────────│
               │
               ▼
      ┌───────────────────┐
      │ HttpRequest::parse│
      │ Extract URI       │
      └─────────┬─────────┘
                │
                ▼
      ┌───────────────────┐
      │ get_mime_type() & │
      │ write() headers   │
      └─────────┬─────────┘
                │
                ▼
        ┌───────────────┐
  ┌────▶│ sendfile()    │ (Zero-Copy File Transfer: loops
  │     └──────┬────────┘  if network buffer fills up)
  │            │
  └────────────│
               │
               ▼
      ┌───────────────────┐
      │ close(client_fd)  │ (Connection Terminated)
      └───────────────────┘
```

**Advanced Pipeline Mechanics:**
* **Edge-Triggered Read Loops:** Because `epoll` is configured in edge-triggered mode, worker threads must read from the socket in a `while(true)` loop until the buffer throws `EAGAIN` or `EWOULDBLOCK`. This guarantees no bytes are left unread in the kernel buffer.
* **Graceful Thread Termination:** The `ThreadPool` destructor flips a boolean kill-switch under a mutex lock, wakes all sleeping threads via `notify_all()`, and safely `join()`s all worker threads before shutdown, ensuring no tasks are abandoned.

---


## Features

- **Asynchronous Multiplexing** — Utilizes Linux `epoll` to monitor thousands of non-blocking file descriptors concurrently on a single thread.
- **Worker Thread Pool** — Decouples I/O monitoring from request processing using a synchronized FIFO task queue managed via condition variable signaling.
- **Zero-Copy File Transfer** — Uses Linux `sendfile()` to transmit static file payloads without an additional user-space copy, reducing CPU overhead and memory copying.
- **Thread-Safe Telemetry** — A centralized `Logger` protected by `std::mutex` prevents console garbling and file-write race conditions from concurrent workers.
- **Edge-Triggered I/O (`EPOLLET`)** — Reduces redundant event notifications by reporting readiness only when socket state changes.

---

## Current Limitations

- HTTP GET requests only
- Linux-only (`epoll` and `sendfile`)
- No HTTPS/TLS
- No HTTP keep-alive
- No chunked transfer encoding
- Static file serving only

---

### Core Components

| Component | Tech | Purpose |
|---|---|---|
| **EpollServer** | `sys/epoll.h`, `sys/socket.h` | The core event loop. Handles socket binding, non-blocking configurations, and event multiplexing. |
| **ThreadPool** | `std::thread`, `std::condition_variable` | Manages a fixed-size pool of worker threads and a task queue to execute incoming requests without blocking the event loop. |
| **Http(Request/Response)** | C++ Strings, `sendfile()` | Parses raw HTTP payloads, maps URIs to the local filesystem, determines MIME types, and executes zero-copy static file transfers using `sendfile()`. |
| **Logger** | `std::mutex`, `std::ofstream` | Thread-safe singleton providing color-coded, timestamped terminal and file logging. |

---


## Architecture
```text
┌──────────────────────────────────────────────────────────────────────┐
│                              USER SPACE                              │
│                                                                      │
│  ┌───────────────┐ 3. Push to ┌────────────┐ 4. Pop fd  ┌──────────┐ │
│  │ EpollServer   │───────────▶│ Task Queue │───────────▶│ Workers  │ │
│  │ (Main Thread) │            │(std::queue)│            │ (Pool)   │ │
│  └───────┬───────┘            └────────────┘            └────┬─────┘ │
│          │ 2. epoll_wait()                                   │       │
│          │    returns ready fd                  5. sendfile()│       │
└──────────┼───────────────────────────────────────────────────┼───────┘
           ▼                                                   ▼
══════════════════════════════ OS BOUNDARY ═════════════════════════════
┌──────────────────────────────────────────────────────────────────────┐
│                             KERNEL SPACE                             │
│                                                                      │
│  ┌───────────────┐                                  ┌──────────────┐ │
│  │ epoll trigger │                                  │ Page Cache   │ │
│  │ (FD Monitor)  │                                  │ (Disk Mem)   │ │
│  └───────────────┘                                  └──────┬───────┘ │
│          ▲                                                 │         │
│          │ 1. Data                 6. sendfile() Transfer  │         │
│          │    Arrives        (ZERO-COPY)                   │         │
│  ┌───────┴───────┐                                         │         │
│  │  TCP Sockets  │◀════════════════════════════════════════╝         │
│  │ (Net Buffers) │                                                   │
│  └───────┬───────┘                                                   │
│          │ ▲                                                         │
└──────────┼─┼─────────────────────────────────────────────────────────┘
  Incoming │ │ Outgoing Responses 
  Requests │ │ (Zero-Copy File Data)
           ▼ │
         ┌───┴──────────┐
         │   CLIENT     │
         │   NETWORK    │
         └──────────────┘
```
---

## Project Structure

```text
vortex-web-server/
├── CMakeLists.txt          # Build configuration (C++17, Threading)
├── include/                # Header definitions
│   ├── concurrency/
│   │   └── ThreadPool.hpp  # Task queue and condition variable logic
│   ├── http/
│   │   ├── Request.hpp     # HTTP parser definition
│   │   └── Response.hpp    # Static file responses and MIME types
│   ├── network/
│   │   └── EpollServer.hpp # Event multiplexer definition
│   └── utils/
│       └── Logger.hpp      # Thread-safe logging definition
├── src/                    # Core Implementation
│   ├── main.cpp            # Server Initialization
│   ├── concurrency/
│   │   └── ThreadPool.cpp  # Worker thread execution loop
│   ├── http/
│   │   ├── Request.cpp     # String parsing and routing
│   │   └── Response.cpp    # Zero-copy static file transfer implementation
│   ├── network/
│   │   └── EpollServer.cpp # Non-blocking socket loop
│   └── utils/
│       └── Logger.cpp      # Mutex-locked terminal and file output
├── public/                 # Static assets served by Vortex
│   ├── index.html          # Dashboard UI
│   └── style.css           # Dashboard styling
├── scripts/                # Automated benchmarking
│   └── benchmark.sh        # ApacheBench load testing script
└── tests/                  # Standalone C++ unit tests
    ├── test_parser.cpp     # Validates HTTP string routing
    └── test_threadpool.cpp # Validates OS thread lifecycle and race conditions
```

---

## Getting Started

### Prerequisites
- **Linux Environment** (Required for Linux `epoll` and `sendfile()` support)
- **C++17 Compiler** (`g++` or `clang++`)
- **CMake** (3.10 or higher)
- **Apache2 Utils** (Optional: required for the `ab` benchmark script)

### Compilation

Vortex uses CMake to manage its build system and link the necessary pthread libraries.

```bash
# Clone the repository
git clone https://github.com/ArjunTomar-G/vortex-web-server.git
cd vortex-web-server

# Generate build files and compile
mkdir build && cd build
cmake ..
make
```
### Execution & Testing

**1. Boot the Server:**
```bash
./vortex_server
```

**2. Run the Unit Tests:**
```bash
./test_parser
./test_pool all
```
---

## Benchmarking

To evaluate the performance of the `epoll` event loop and the zero-copy file transfer, Vortex includes a standardized benchmarking script using ApacheBench (`ab`). 

### The Crucible Test
The target benchmark fires **50,000 total requests** with a **concurrency of 1,000 simultaneous connections**. 

Because of the architectural decoupling, the main thread continues accepting new connections while worker threads process requests and static file payloads are transmitted using `sendfile()`.
**Run the Benchmark (while the server is running):**
```bash
bash scripts/benchmark.sh
```

### Expected Benchmark Output

When running the benchmark, ApacheBench produces output similar to the following:

```text
==========================================
       VORTEX ENGINE: LOAD TESTER         
==========================================
[INFO] Target Engine: http://127.0.0.1:8080/
[INFO] Verifying server status...
[INFO] Server is online. Initiating warm-up phase...
[INFO] Warm-up complete. Page cache primed.

[INFO] Commencing Maximum Throughput Test...
[INFO] Payload: 50,000 Requests | 1,000 Concurrent Connections
------------------------------------------
This is ApacheBench, Version 2.3 <$Revision: 1923142 $>
Copyright 1996 Adam Twiss, Zeus Technology Ltd, http://www.zeustech.net/
Licensed to The Apache Software Foundation, http://www.apache.org/

Benchmarking 127.0.0.1 (be patient)
Completed 5000 requests
Completed 10000 requests
Completed 15000 requests
Completed 20000 requests
Completed 25000 requests
Completed 30000 requests
Completed 35000 requests
Completed 40000 requests
Completed 45000 requests
Completed 50000 requests
Finished 50000 requests


Server Software:        Vortex
Server Hostname:        127.0.0.1
Server Port:            8080

Document Path:          /
Document Length:        3763 bytes

Concurrency Level:      1000
Time taken for tests:   1.576 seconds
Complete requests:      50000
Failed requests:        0
Total transferred:      193200000 bytes
HTML transferred:       188150000 bytes
Requests per second:    31727.10 [#/sec] (mean)
Time per request:       31.519 [ms] (mean)
Time per request:       0.032 [ms] (mean, across all concurrent requests)
Transfer rate:          119720.21 [Kbytes/sec] received

Connection Times (ms)
              min  mean[+/-sd] median   max
Connect:        0    1   2.3      1      19
Processing:    15   30   6.2     29      61
Waiting:        0   29   5.9     29      58
Total:         19   31   6.2     30      67

Percentage of the requests served within a certain time (ms)
  50%     30
  66%     32
  75%     33
  80%     34
  90%     37
  95%     40
  98%     52
  99%     60
 100%     67 (longest request)
------------------------------------------
[INFO] Stress test concluded.
==========================================
```
### Trace Execution Metrics Analysis

* **Event-Driven Scalability:** The server handles the entire 1,000-connection high-concurrency workload with **0 failed requests**, showing that the edge-triggered `epoll` efficiently handles large numbers of concurrent connections while minimizing unnecessary event processing.
* **Zero-Copy File Transfer:** Clocking over **31,000+ Requests Per Second** at an active transfer rate of `~119.7 MB/s` demonstrates that `sendfile()` efficiently serves static file payloads while avoiding an additional copy through user space, reducing CPU overhead associated with traditional file-copying paths.