#include "engine.h"
#include "device_abstractor.h"

#define BUFFER_LEN 2048
const uint32_t BUFFER_FILL_THRESHOLD = 512;

namespace Audio {

bool Engine::fillBuffer(const uint16_t fill_frames) 
{
    float tmp[fill_frames];
    file_io.readFrames(fill_frames, tmp);
    for(int i = 0; i < fill_frames; i++) {
        sample s;
        s.value = tmp[i];
        buffer.push(s);
    }
    return true;
}

void Engine::monitorAudioBufferLevel() 
{
    while (running) 
    {
        if (buffer.size() < BUFFER_FILL_THRESHOLD) 
        {
            uint32_t fill = buffer_fill_level.load(std::memory_order_acquire);
            fillBuffer(fill);
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(wait_time));
        }
    }
}

void Engine::processAudio(float* output_buffer, ma_uint32 frame_count) 
{
    size_t remaining_frames = size_of_audio / file_io.file.channels() - current_frame;
    size_t frames_to_copy = std::min((size_t)frame_count, remaining_frames);
    if (running) 
    {
        buffer_fill_level.store(frames_to_copy * file_io.file.channels(), std::memory_order_release);

        if (frames_to_copy > 0) {
            for (size_t i = 0; i < frames_to_copy * file_io.file.channels(); i++) {
                sample frame;
                buffer.pop(frame);
                output_buffer[i] = frame.value;
            }
            current_frame += frames_to_copy;
        }
        
        if (frames_to_copy < frame_count) {
            std::fill_n(output_buffer + frames_to_copy * file_io.file.channels(), 
                        (frame_count - frames_to_copy) * file_io.file.channels(), 
                        0.0f);
        }
    }
    else
    {
        std::fill_n(output_buffer + frames_to_copy * file_io.file.channels(), 
                        (frame_count - frames_to_copy) * file_io.file.channels(), 
                        0.0f);
    }
}

Engine::Engine() {}
Engine::~Engine() {
    stop();
}

bool Engine::initialize(const std::string& filename) {
    // Open audio file
    if (!file_io.openAudioFile(filename)) {
        std::cerr << "Failed to open audio file." << std::endl;
        return false;
    }

    size_of_audio = file_io.file.frames() * file_io.file.channels();

    // Pre-fill buffer 
    fillBuffer(BUFFER_LEN);
    
    // Initialize DeviceAbstractor thing, give it the process call back
    device = std::make_unique<Audio::DeviceAbstractor>();
    ProcessAudioCallback process_audio_callback = [this](float* output_buffer, ma_uint32 frame_count) {
        this->processAudio(output_buffer, frame_count);
    };
    device->setCallback(process_audio_callback);

    device->initialize(file_io.file.samplerate(), file_io.file.channels());
    wait_time = static_cast<uint32_t>(std::round((((BUFFER_FILL_THRESHOLD/2) * ( 1.0 / file_io.file.samplerate()) * 1000))));
    return true;
}

void Engine::start() {
    running = true;
    io_thread = std::thread(&Engine::monitorAudioBufferLevel, this);
    device->start();
}

void Engine::stop() {
    running = false;
    if (io_thread.joinable()) {
        io_thread.join();
    }
    device->stop();
}

}