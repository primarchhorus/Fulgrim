#pragma once

#include <sndfile.hh>

namespace Audio {
    constexpr uint32_t CIRCULAR_BUFFER_SIZE = 256;
    struct FileIO {

        FileIO();
        ~FileIO();
        FileIO(const FileIO& other) = default;
        FileIO(FileIO&& other) noexcept = default;
        FileIO& operator=(const FileIO& other) = default;
        FileIO& operator=(FileIO&& other) noexcept = default;

        bool openAudioFile(const std::string& filename);
        sf_count_t readFrames(const uint64_t frames, float* data);
        sf_count_t seekToFrame(const uint64_t frame);

        SndfileHandle file;
    };
}
