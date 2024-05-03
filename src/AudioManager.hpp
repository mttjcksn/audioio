#pragma once

#include <functional>
#include <string>
#include <map>

class AudioDeviceDescriptor
{
public:
    std::string name = "";
    int samplesRate = 48000;
    int maxInputChannels = 0;
    int maxOutputChannels = 0;
};

template <typename T>
class AudioManager
{
public:
    AudioManager()
    {
    }

    ~AudioManager()
    {
        delete mSampleInputBuffer;
        delete mSampleOutputBuffer;
    }

    /**
     * @brief Configure the audio IO
     *
     * @param sampleRate Sample rate to run input and output at
     * @param bufferSize Number of samples to process each callback
     * @param maxInputChannels Max number of input channels
     * @param maxInputChannels Max number of output channels
     */
    void configure(int sampleRate, uint32_t bufferSize, int maxInputChannels, int maxOutputChannels)
    {
        mSampleRate         = sampleRate;
        mBufferSize         = bufferSize;
        mMaxInputChannels = maxInputChannels;
        mMaxOutputChannels = maxOutputChannels;
        mSampleInputBuffer  = new T[2 * mBufferSize * mMaxInputChannels];
        mSampleOutputBuffer = new T[2 * mBufferSize * mMaxOutputChannels];
        memset(mSampleInputBuffer, 0, sizeof(T) * 2 * mBufferSize * mMaxInputChannels);
        memset(mSampleOutputBuffer, 0, sizeof(T) * 2 * mBufferSize * mMaxOutputChannels);
    }

    virtual bool startAudio() = 0;
    virtual bool stopAudio()  = 0;

    virtual std::map<int, AudioDeviceDescriptor> enumerateDevices() = 0;

    /**
     * @brief 
     * 
     * @param deviceIndex Device index obtained from enumerateDevices. Use -1 for default device.
     * @param numChannels Desired number of channels.
     * @return true 
     * @return false 
     */
    virtual bool selectInputDevice(int deviceIndex, int numChannels) = 0;
    virtual bool selectOutputDevice(int deviceIndex, int numChannels) = 0;

    /**
     * @brief Used by app to get output buffer to fill
     *
     * @return Pointer to output buffer of size mBufferSize to fill
     */
    T *getOutputBuffer()
    {
        if (mBufferSide)
        {
            return mSampleOutputBuffer + mBufferSize;
        }
        return mSampleOutputBuffer;
    }

    /**
     * @brief Used by app to get input buffer to read
     *
     * @return Pointer to buffer of size mBufferSize to read
     */
    T *getInputBuffer()
    {
        if (mBufferSide)
        {
            return mSampleInputBuffer + mBufferSize;
        }
        return mSampleInputBuffer;
    }

    /**
     * @brief Set the onAudio callback. This will be called every time new audio is ready to be processed.
     *
     * @param func Callback function
     */
    void setOnAudioCallback(std::function<void()> func)
    {
        mOnAudioCallback = func;
    }

protected:

    uint32_t mSampleRate = 48000;
    int mBufferSize      = 128;
    int mMaxInputChannels = 1;
    int mMaxOutputChannels = 1;

    /**
     * @brief Copies samples between local buffers and host buffers.
     *          To be called by derived classes.
     *
     * @param inputBuffer Incoming samples from host
     * @param outputBuffer Outoing samples to host
     */
    void onSamples(T *inputBuffer, T *outputBuffer)
    {
        T *outBuff = getOutputBuffer();
        memcpy(getInputBuffer(), inputBuffer, mBufferSize * sizeof(T));
        memcpy(outputBuffer, outBuff, mBufferSize * sizeof(T));
        memset(outBuff, 0, mBufferSize * sizeof(T));
        updateBuffers();
        if (mOnAudioCallback)
        {
            mOnAudioCallback();
        }
    }

private:
    /**
     * @brief Switches ping-pong buffer sides
     */
    void updateBuffers()
    {
        mBufferSide = !mBufferSide;
    }

    bool mBufferSide       = true;
    T *mSampleInputBuffer  = nullptr;       ///< Input from host
    T *mSampleOutputBuffer = nullptr;       ///< Output to host
    std::function<void()> mOnAudioCallback; ///< Callback on data
};