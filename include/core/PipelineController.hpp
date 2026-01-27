#ifndef PIPELINECONTROLLER_HPP_
#define PIPELINECONTROLLER_HPP_

#include "PacketStore.hpp"
#include "PacketCapture.hpp"
#include "ThreadPool.hpp"
#include "PacketProcessor.hpp"

#include <mutex>

/**
 * @brief Coordinates the entire packet processing pipeline.
 *
 * PipelineController manages the flow of packets from capture to storage:
 *   Capture -> RawQueue -> Dispatcher -> ThreadPool -> Processor -> Store
 */
class PipelineController {
public:
    PipelineController();
    ~PipelineController();

    PipelineController(const PipelineController&) = delete;
    PipelineController& operator=(const PipelineController&) = delete;
    PipelineController(PipelineController&&) = delete;
    PipelineController& operator=(PipelineController&&) = delete;

    /**
     * @brief Starts or resumes the capture pipeline.
     *
     * If called after stop(), resumes capture on the specified device.
     * Existing packets in store are preserved.
     *
     * @param deviceName Network device name (eth0, wlan0, etc.)
     */
    bool start(const std::string& deviceName);

    /**
     * @brief Pauses the capture pipeline.
     *
     * Stops capturing new packets. Existing packets remain in store.
     * Can be resumed with start().
     *
     * Shutdown sequence:
     *  Stop packet capture (no new packets produced)
     *  Push poison pill (std::nullopt) to raw packet queue
     *  Dispatcher thread drains queue and exits
     *  ThreadPool processes all submitted tasks and shuts down
     */
    void stop();

    /**
     * @brief Restarts the pipeline on the current device.
     *
     * Clears all stored packets and statistics, then starts
     * fresh capture on the same device.
     */
    bool restart();

    /**
     * @brief Checks if pipeline is currently running.
     * @return true if running, false otherwise
     */
    bool isRunning() const;

    /**
     * @brief Lists available network devices for capture.
     * @return Vector of device information (name, description)
     */
    std::vector<packetscope::DeviceInfo> listAvailableDevices() const;

    /**
     * @brief Returns the packet store for UI access.
     * UI can read packets from this store while capture is running.
     * @return Shared pointer to PacketStore
     */
    std::shared_ptr<PacketStore> getStore() const;

    /**
     * @brief Returns current raw packet queue size.
     * Useful for monitoring backpressure and burst detection.
     * @return Number of packets waiting to be processed
     */
    std::size_t queueSize() const;

    /**
     * @brief Returns total captured packet count since start
     * @return Number of packets captured since start
     */
    std::size_t capturedCount() const;

    /**
     * @brief Returns total processed packet count.
     * @return Number of packets in store
     */
    std::size_t processedCount() const;

private:

    // Packet storage (shared with UI)
    std::shared_ptr<PacketStore> packetStore_;

    // Packet capture (PcapPlusPlus wrapper)
    std::unique_ptr<PacketCapture> packetCapture_;

    // Worker thread pool
    std::unique_ptr<ThreadPool> threadPool_;

    // Stateless packet parser
    PacketProcessor packetProcessor_;

    /**
     * Buffer between capture and processing
     *
     * NOTE:
     * rawPacketQueue_ uses a poison pill shutdown mechanism.
     * std::nullopt is used exclusively as a termination signal
     * for the dispatcher thread. Only a single consumer
     * (dispatcherThread_) is expected.
     */
    ThreadSafeQueue<std::optional<packetscope::RawPacketData>> rawPacketQueue_;

    // Dispatcher thread (queue -> pool)
    std::thread dispatcherThread_;

    // PipelineController state flag
    std::atomic<bool> isRunning_{false};

    // Mutex for start/stop coordination
    std::mutex controlMutex_;

    // Worker thread count
    static constexpr std::size_t kWorkerCount = 2;

    // Current device name (for restart)
    std::string currentDeviceName_;
};

#endif