## PacketScope

A network packet analyzer with a Qt based GUI, designed for real-time packet capture, parsing and inspection.

<p align="center">
  <img src="https://github.com/user-attachments/assets/14c0fe53-28dc-4ab1-9579-c5ca262f60e7" width="45%" />
  <img src="https://github.com/user-attachments/assets/d445e30f-51ec-4d61-b2e8-2ab80a8d550b" width="45%" />
</p>

### Dependencies
- **Qt6** (6.10.1)
- **PcapPlusPlus** (25.05)
- **CMake** (3.16 or later)

### Building

1) Install dependencies
2) Configure Qt Path
Before started to build you must edit `CMakeLists.txt` and set the correct Qt path for your host:
  `set(CMAKE_PREFIX_PATH "/path/to/Qt/6.10.1")`

4) Build

    ```mkdir build && cd build && cmake .. && make```

    It will build for Release build type. If you want to Debug, you must use:

    ```mkdir build && cd build && cmake ..-DCMAKE_BUILD_TYPE=DEBUG && make```
  
    Optional: If you want to enable sanitizers for debugging, you can simply add
      `-DENABLE_ASAN=ON` or `-DENABLE_TSAN=ON`  
    Optional: If you want to build documentation you might be consider to enable
     `-DBUILD_DOCS=ON`

6) Run

    ```sudo ./packet-scope```

## Architecture

### Pipeline Flow

```
PacketCapture                   [Capture Thread]
|
| RawPacketData
v
ThreadSafeQueue<RawPacketData>  [Blocking Buffer]
|
| pop()
v
Dispatcher Loop                 [Dispatcher Thread, in PipelineController]
|
| task submit()
v
ThreadPool                      [Worker Threads]
|
| PacketProcessor::process()
v
PacketStore                     [shared_mutex, owned by PipelineController]
|
| ParsedPacket (shared_lock read)
v
PacketListModel -> MainWindow    [Main Thread]
```

### Strategy

1. **Raw Packet Queue** (Producer-Consumer)
   - Producer: Capture Thread (single)
   - Consumer: Dispatcher Thread (single)
   - Synchronization: `std::mutex` + `std::condition_variable`
   - Shutdown: Poison pill with `std::nullopt`

2. **Task Queue** (ThreadPool)
   - Producer: Dispatcher Thread (single)
   - Consumers: Worker Threads (multiple)
   - Synchronization: ThreadSafeQueue (same as above)
   - Shutdown: Poison pill with empty `std::function<void()>`

3. **PacketStore** (Reader-Writer)
   - Writers: Worker Threads (multiple but `shared_mutex` ensures exclusivity)
   - Readers: Main Thread (UI refresh via `PacketListModel`)
   - Synchronization: `std::shared_mutex`
   - Lock Strategy:
     - `addPacket()`: Exclusive lock (one writer at a time)
     - `getById()`, `count()`: Shared lock (multiple readers)

### Shutdown Sequence

1) Stop PacketCapture  
    -> No new packets produced

2) Push poison pill (std::nullopt) to raw queue  
    -> Signals dispatcher to exit

3) Dispatcher thread drains queue  
    -> Submits all pending packets to ThreadPool  
    -> Exits on receiving poison pill

5) Join dispatcher thread  
    -> Ensures all packets are submitted

7) ThreadPool shutdown  
    -> Stops accepting new tasks  
    -> Pushes empty function (poison pill) per worker  
    -> Workers drain their task queue  
    -> Workers exit on empty function  

9) Join all worker threads  
    -> Pipeline fully stopped
