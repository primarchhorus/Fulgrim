#include "file_io.h"

namespace Audio {
    FileIO::FileIO() {

    }

    FileIO::~FileIO() {

    }

    bool FileIO::openAudioFile(const std::string& filename) {
        file = SndfileHandle(filename);
        if (file)
        {
            return true;
        }
        return false;
    }

    sf_count_t FileIO::readFrames(const uint64_t frames, float* data) {
        sf_count_t ret = 0;
        ret = file.read(data, frames);
        return ret;
    }

    sf_count_t FileIO::seekToFrame(const uint64_t frame) {
        sf_count_t ret = 0;
        ret = file.seek(frame, SEEK_SET);
        return ret;
    }
}