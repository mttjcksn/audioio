#pragma once

#include "Tui.hpp"

class App
{
public:
    static App *getInstance();

    App();
    ~App();

    int run();
    void onSamples();

private:

    const int mkAudioBufferSize = 256;
    Tui mTui;
};
