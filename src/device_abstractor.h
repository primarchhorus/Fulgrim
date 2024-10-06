#pragma once

#include <functional>
#include <iostream>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

namespace Audio
{
    using ProcessAudioCallback = std::function<void(float *, ma_uint32)>;
    class DeviceAbstractor
    {
    private:
        ma_device device;
        ProcessAudioCallback process_audio_func;
        ma_device_config config;

    public:
        DeviceAbstractor() {};

        ~DeviceAbstractor()
        {
            stop();
        };

        bool start()
        {
            if (ma_device_start(&device) != MA_SUCCESS)
            {
                std::cerr << "Failed to start audio device." << std::endl;
                stop();
            }
            return true;
        };

        void stop()
        {
            ma_device_stop(&device);
            ma_device_uninit(&device);
        };

        bool initialize(uint32_t sampleRate, uint32_t channels)
        {
            config = ma_device_config_init(ma_device_type_playback);
            config.playback.format = ma_format_f32;
            config.sampleRate = sampleRate;
            config.playback.channels = channels;
            config.dataCallback = &staticDataCallback;
            config.pUserData = this;
            if (ma_device_init(NULL, &config, &device) != MA_SUCCESS)
            {
                std::cerr << "Failed to initialize audio device." << std::endl;
                return false;
            }
            return true;
        };

        void dataCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
        {
            process_audio_func(static_cast<float *>(output), frame_count);
        };

        static void staticDataCallback(ma_device *device, void *output, const void *input, ma_uint32 frame_count)
        {
            auto *self = static_cast<DeviceAbstractor *>(device->pUserData);
            self->dataCallback(device, output, input, frame_count);
        };

        void setCallback(ProcessAudioCallback _process_audio_func)
        {
            process_audio_func = _process_audio_func;
        }
    };
}