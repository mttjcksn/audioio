#include "App.h"

#include <iostream>
#include <limits>

#include "PortAudioManager.hpp"
#define AUDIO_MANAGER (PortAudioManager<int32_t>::instance())


App *App::getInstance()
{
    static App *gAppInstance = nullptr;
    if (gAppInstance == nullptr)
    {
        gAppInstance = new App;
    }
    return gAppInstance;
}

App::App()
{
}

App::~App()
{
}

void App::onSamples()
{
    int32_t* samps = AUDIO_MANAGER->getInputBuffer();

    int32_t* maxElement = std::max_element(samps, samps + sizeof(mkAudioBufferSize) / sizeof(int32_t), [](float a, float b) { return std::abs(a) < std::abs(b); });

    double normalisedValue = static_cast<double>(*maxElement) / static_cast<float>(std::numeric_limits<int32_t>::max());
    double dbfsValue = 20 * std::log10(std::fabs(normalisedValue));
    float level = static_cast<float>(dbfsValue);
    mTui.updateMeter(0, level);
}

int App::run()
{
    AUDIO_MANAGER->setOnAudioCallback(std::bind(&App::onSamples, this));

    AUDIO_MANAGER->configure(48000, mkAudioBufferSize, 1, 1);
    
    mTui.setDevices(AUDIO_MANAGER->enumerateDevices());

    auto onRefreshDevicesClick = [this]() { mTui.setDevices(AUDIO_MANAGER->enumerateDevices()); };
    auto onOpen = [this](int deviceIndex, bool open) 
    { 
        AUDIO_MANAGER->selectOutputDevice(-1, 1);

        if(!AUDIO_MANAGER->selectInputDevice(deviceIndex, 1))
        {
            mTui.setStatus("Error selecting");
        }

        if(open)
        {
            if(!AUDIO_MANAGER->startAudio()){mTui.setStatus("Error opening device!");}else{mTui.setDeviceOpened();}
        }
        else
        {
            if(!AUDIO_MANAGER->stopAudio()){mTui.setStatus("Error closing device!");}else{mTui.setDeviceClosed();}
        }
    };

    mTui.connectToRefreshSignal(onRefreshDevicesClick);
    mTui.connectToOpenSignal(onOpen);
    mTui.runLoop();

    while(1);

    return 0;
}