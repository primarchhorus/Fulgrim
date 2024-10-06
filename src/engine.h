#pragma once

#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include "audio_types.h"
#include "circular_buffer.hpp"
#include "file_io.h"

constexpr uint32_t BUFFER_LEN = 2048;

namespace Audio {
// Forward declaring this, because if the miniaudio header is included, it will cause a redefinition error
class DeviceAbstractor;

class Engine {
private:
    CircularBuffer<sample, BUFFER_LEN> buffer;
    FileIO file_io;
    size_t size_of_audio{0};
    size_t current_frame{0};
    std::atomic<uint32_t> buffer_fill_level{BUFFER_LEN};
    std::atomic_bool running{false};
    std::thread io_thread;
    uint32_t wait_time{10};
    std::unique_ptr<DeviceAbstractor> device;
    bool fillBuffer(const uint16_t fill_frames);
    void monitorAudioBufferLevel();
    void processAudio(float* output_buffer, uint32_t frame_count);

public:
    Engine();
    ~Engine();

    Engine(const Engine& other) = delete;
    Engine(Engine&& other) noexcept = delete;
    Engine& operator=(const Engine& other) = delete;
    Engine& operator=(Engine&& other) noexcept = delete;

    bool initialize(const std::string& filename);
    void start();
    void stop();
};

} // namespace Audio