#pragma once

#include "AudioManager.hpp"
#include "portaudio.h"

#include <stdint.h>
#include <type_traits>

template <typename T>
class PortAudioManager : public AudioManager<T>
{
public:
    static PortAudioManager *instance()
    {
        static PortAudioManager *gAudioManagerInstance = nullptr;
        if (gAudioManagerInstance == nullptr)
        {
            gAudioManagerInstance = new PortAudioManager;
        }
        return gAudioManagerInstance;
    }

    PortAudioManager()
    {
        Pa_Initialize();
    }

    ~PortAudioManager()
    {
        Pa_Terminate();
    }

    bool selectInputDevice(int deviceIndex, int numChannels) override
    {
        if(!numChannels){return false;}
        if(deviceIndex == -1){deviceIndex = Pa_GetDefaultInputDevice();}
        auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        if(deviceInfo == NULL){return false;}
    
        numChannels = std::clamp(numChannels, 1, AudioManager<T>::mMaxInputChannels); // Clamp to between 1 and configured max num channels
        numChannels = std::min(numChannels, deviceInfo->maxInputChannels); // Don't use more channels than the device can provide

        mInputStreamParams.device                    = deviceIndex;
        mInputStreamParams.channelCount              = numChannels;
        mInputStreamParams.sampleFormat              = getPaSampleFormat();
        mInputStreamParams.suggestedLatency          = Pa_GetDeviceInfo(mInputStreamParams.device)->defaultLowInputLatency;
        mInputStreamParams.hostApiSpecificStreamInfo = NULL;

        return true;
    }

    bool selectOutputDevice(int deviceIndex, int numChannels) override
    {
        if(!numChannels){return false;}
        if(deviceIndex == -1){deviceIndex = Pa_GetDefaultOutputDevice();}
        auto deviceInfo = Pa_GetDeviceInfo(deviceIndex);
        if(deviceInfo == NULL){return false;}
    
        numChannels = std::clamp(numChannels, 1, AudioManager<T>::mMaxOutputChannels); // Clamp to between 1 and configured max num channels
        numChannels = std::min(numChannels, deviceInfo->maxOutputChannels); // Don't use more channels than the device can provide

        mOutputStreamParams.device                    = deviceIndex;
        mOutputStreamParams.channelCount              = numChannels;
        mOutputStreamParams.sampleFormat              = getPaSampleFormat();
        mOutputStreamParams.suggestedLatency          = Pa_GetDeviceInfo(mOutputStreamParams.device)->defaultLowOutputLatency;
        mOutputStreamParams.hostApiSpecificStreamInfo = NULL;

        return true;
    }



    std::map<int, AudioDeviceDescriptor> enumerateDevices() override
    {
        std::map<int, AudioDeviceDescriptor> devices;
        int numDevices;

        numDevices = Pa_GetDeviceCount();

        if(numDevices > 0)
        {
            const PaDeviceInfo *deviceInfo;
            for(int i=0; i<numDevices; i++)
            {
                deviceInfo = Pa_GetDeviceInfo(i);
                devices[i] = {deviceInfo->name, (int)deviceInfo->defaultSampleRate, deviceInfo->maxInputChannels, deviceInfo->maxOutputChannels};
            }
        }

        return devices;
    }

    bool startAudio() override
    {   
        PaError err = Pa_OpenStream(
            &mpStream,
            &mInputStreamParams,
            &mOutputStreamParams,
            this->mSampleRate,
            this->mBufferSize,
            paClipOff, /* we won't output out of range samples so don't bother clipping them */
            &streamCallback,
            this);

        if (err != paNoError)
        {
            return false;
        }

        err = Pa_StartStream(mpStream);
        if (err != paNoError)
        {
            return false;
        }
        return true;
    }

    bool stopAudio() override
    {
        PaError err = Pa_StopStream(mpStream);
        if (err != paNoError)
        {
            return false;
        }

        err = Pa_CloseStream(mpStream);

        if (err != paNoError)
        {
            return false;
        }
        
        return true;
    }

private:

    PaStream *mpStream = nullptr;
    PaStreamParameters mOutputStreamParams;
    PaStreamParameters mInputStreamParams;

    constexpr PaSampleFormat getPaSampleFormat()
    {
        static_assert(std::is_same_v<T, float> || std::is_same_v<int32_t, int32_t> || std::is_same_v<T, int16_t>, "Unsupported type");
        if constexpr (std::is_same_v<T, float>)
            return paFloat32;
        else if constexpr (std::is_same_v<int32_t, int32_t>)
            return paInt32;
        else if constexpr (std::is_same_v<T, int16_t>)
            return paInt16;
    }

    static int streamCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long framesPerBuffer,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData)
    {
        PortAudioManager *amInstance = static_cast<PortAudioManager *>(userData);
        amInstance->onSamples((T *)inputBuffer, (T *)outputBuffer);
        return paContinue;
    };

};