#pragma once

#include <atomic>
#include <cstdint>

template <typename T>
class RingBuffer
{
public:
    RingBuffer(std::size_t samples) : mBufferSize(samples)
    {
        mpBufferStart = new T[samples];
        mpBufferEnd = mpBufferStart + mBufferSize;
        mpHead = mpBufferStart;
        mpTail = mpBufferStart;
        mRingBuffMutex.unlock();
    }

    /**
     * @brief Returns number of unread elements in buffer
     *
     * @return uint32_t Number of elements available in buffer
     */
    uint32_t available()
    {
        if (mpBufferStart == nullptr)
        {
            return 0;
        }
        if (mpHead >= mpTail)
        {
            return static_cast<uint32_t>(mpHead - mpTail);
        }
        else
        {
            return mBufferSize - static_cast<uint32_t>(mpTail - mpHead);
        }
    }

    /**
     * @brief
     *
     * @param data Pointer to data to write to buffer
     * @param n Number of elements to write
     * @return true if data is successfully written
     * @return false is not enough room in buffer
     */
    bool write(T *data, uint32_t n)
    {
        std::lock_guard<std::mutex> lock(mRingBuffMutex);

        uint32_t spaceBeforeWrap = static_cast<uint32_t>(mpBufferEnd - mpHead);
        uint32_t nToWrite = n;

        if ((mBufferSize - available()) < n)
            return false;

        // If we need to handle a wrap, copy bytes in up until the end of the buffer
        if (n > spaceBeforeWrap)
        {
            memcpy(mpHead, data, spaceBeforeWrap * sizeof(T));
            mpHead = mpBufferStart;
            data += spaceBeforeWrap;
            nToWrite -= spaceBeforeWrap;
        }

        // Copy all or remaining bytes
        memcpy(mpHead, data, nToWrite * sizeof(T));
        mpHead += nToWrite;

        return true;
    }

    /**
     * @brief Reads a specified number of elements from the buffer
     *
     * @return Vector of elements read from buffer or empty vector if not enough elements
     */
    std::vector<T> read(uint32_t n)
    {
        std::lock_guard<std::mutex> lock(mRingBuffMutex);

        uint32_t readsBeforeWrap = static_cast<uint32_t>(mpBufferEnd - mpTail);
        std::vector<T> result;

        if (available() < n)
            return result;

        // If we need to handle a wrap, read bytes up until the end of the buffer
        if (n > readsBeforeWrap)
        {
            result.insert(result.end(), mpTail.load(), mpBufferEnd);
            mpTail = mpBufferStart;
            n -= readsBeforeWrap;
        }

        // Read all or remaining bytes
        result.insert(result.end(), mpTail.load(), mpTail.load() + n);
        mpTail += n;

        return result;
    }

    /**
     * @brief Reads a specified number of elements from the buffer into another buffer
     *
     * @return True if there was enough data to be read, false otherwise
     */
    bool readIntoBuffer(T* buffer, uint32_t n)
    {
        std::lock_guard<std::mutex> lock(mRingBuffMutex);

        uint32_t readsBeforeWrap = static_cast<uint32_t>(mpBufferEnd - mpTail);
        T* bufferHead = buffer;

        if (available() < n)
            return false;

        // If we need to handle a wrap, read bytes up until the end of the buffer
        if (n > readsBeforeWrap)
        {
            memcpy(bufferHead, mpTail, readsBeforeWrap * sizeof(T));
            mpTail = mpBufferStart;
            n -= readsBeforeWrap;
            bufferHead += readsBeforeWrap;
        }

        // Read all or remaining bytes
        memcpy(bufferHead, mpTail, n * sizeof(T));
        mpTail += n;

        return true;
    }

    bool discardSamples(uint32_t n)
    {
        std::lock_guard<std::mutex> lock(mRingBuffMutex);

        uint32_t readsBeforeWrap = static_cast<uint32_t>(mpBufferEnd - mpTail);

        if (available() < n)
            return false;

        // If we need to handle a wrap, read bytes up until the end of the buffer
        if (n > readsBeforeWrap)
        {
            mpTail = mpBufferStart;
            n -= readsBeforeWrap;
        }

        // Advance remaining amount
        mpTail += n;

        return true;
    }

    /**
     * @brief Clears the audio buffer
     *
     */
    void reset()
    {
        std::lock_guard<std::mutex> lock(mRingBuffMutex);
        mpTail = mpHead = mpBufferStart;
    }

    /**
     * @brief Reads a specified number of elements from the buffer but only consumes the strides
     *
     * @return Vector of elements read from buffer or empty vector if not enough elements
     */
    std::vector<T> readWindow(int windowSize, int stride)
    {
        // Do a normal read
        T *pTail = mpTail;
        std::vector<T> result = read(windowSize);

        if (!result.size())
            return result;

        mpTail = pTail;
        uint32_t readsBeforeWrap = static_cast<uint32_t>(mpBufferEnd - mpTail);

        if (stride >= readsBeforeWrap)
        {
            mpTail = mpBufferStart;
            stride -= readsBeforeWrap;
        }

        mpTail += stride;

        return result;
    }

private:
    const uint32_t mBufferSize = 0;
    T *mpBufferStart = nullptr;
    T *mpBufferEnd;
    std::atomic<T *> mpHead;
    std::atomic<T *> mpTail;
    std::mutex mRingBuffMutex;
};