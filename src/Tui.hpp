#pragma once

#include <string>
#include <vector>
#include <functional>

#include "ftxui/component/component.hpp"          // for Radiobox, Renderer
#include "ftxui/component/screen_interactive.hpp" // for ScreenInteractive
#include "ftxui/dom/node.hpp"                     // for Render
#include <ftxui/dom/elements.hpp>                 // for graph, operator|, separator, color, Element, vbox, flex, inverted, operator|=, Fit, hbox, size, border, GREATER_THAN, HEIGHT
#include <ftxui/screen/screen.hpp>                // for Full, Screen

#include "nano_signal_slot.hpp"

#include "AudioManager.hpp"

#include <iostream>

using namespace ftxui;

class Tui
{
public:
    Tui(){};

    ~Tui(){};

    /**
     * @brief Set the list of available audio devices
     * 
     * @param std::map of AudioDeviceDescriptor by index
     */
    void setDevices(std::map<int, AudioDeviceDescriptor> devs)
    {
        mDeviceNames.clear(); 

        for (const auto& pair : devs)
        {
            const AudioDeviceDescriptor& descriptor = pair.second;
            mDeviceNames.push_back(descriptor.name);
        }

        if (ScreenInteractive::Active())
        {
            ScreenInteractive::Active()->Post(Event::Custom);
        }
    }

    /**
     * @brief Set the status text
     * 
     * @param status 
     */
    void setStatus(std::string status)
    {
        mStatus = status;
        ScreenInteractive::Active()->Post(Event::Custom);
    }

    void setDeviceOpened()
    {
        mStatus               = "Device open";
        mDeviceIsOpen         = true;
    }

    void setDeviceClosed()
    {
        mStatus               = "Device closed";
        mDeviceIsOpen         = false;
    }


#pragma GCC diagnostic ignored "-Wunused-parameter"
    void updateMeter(int channel, float dBFS)
    {
        static const float min_dbfs = -48.0;
        static const float max_dbfs = 0.0;
        static const float lpAlpha = 0.7;
        static float smoothedLevel = min_dbfs;

        // Smooth and scale to 0-1 for UI component
        dBFS = std::clamp(dBFS, min_dbfs, max_dbfs);
        smoothedLevel = lpAlpha * dBFS + (1.0 - lpAlpha) * smoothedLevel;
        mChannel0Level = (smoothedLevel - min_dbfs) / (max_dbfs - min_dbfs);
        ScreenInteractive::Active()->Post(Event::Custom);
    }

    void connectToRefreshSignal(std::function<void()> func) { refreshButtonSignal.connect(func); }
    void connectToOpenSignal(std::function<void(int, bool)> func) { openButtonSignal.connect(func); }

    void runLoop()
    {
        auto composition = Container::Vertical({ pane0, Container::Horizontal({ pane1, pane2, meteringPane }) });
        auto screen      = ScreenInteractive::FitComponent();
        screen.Loop(composition);
    }

private:


    const int mkDeviceSelectWidth = 32;

    std::vector<std::string> mDeviceNames;
    int mSelectedDevice        = 0;
    bool mDeviceIsOpen  = false;
    std::string mStatus = "Initialised";
    float mChannel0Level = 0.0f;

    std::mutex mGraphDataMutex;
    std::vector<int> mGraphData = std::vector<int>(100, 2);

    // Signals
    Nano::Signal<void(int, bool)> openButtonSignal;
    Nano::Signal<void()> refreshButtonSignal;


    const Component title        = Renderer([]{ return text("audioio"); });
    const Component status       = Renderer([&]{ return text(mStatus); });
    const Component seperator    = Renderer([]{ return separator(); });
    const Component levelMeter   = Renderer([&]{return border(gaugeUp(mChannel0Level));});

    Component dropDown = Dropdown({
        .radiobox = {.entries = &mDeviceNames, .selected = &mSelectedDevice, .on_change = [this]{this->setStatus("Selected " + mDeviceNames.at(mSelectedDevice));}},
        .checkbox = {.label = getDevDropdownLabel(), 
            .transform = [&](const EntryState& s) {
            auto prefix = text(s.state ? "↓ " : "→ ");  // NOLINT
            auto t = text(getDevDropdownLabel());
            if (s.active) {
                t |= bold;
            }
            if (s.focused) {
                t |= inverted;
            }
            return hbox({prefix, t});
            }
        }
    });

    Component openButton      = Button("Open", [this]{ this->toggleDevice(); }, openCloseButtonStyle());

    // std::function<std::vector<int>(int, int)> func = [this](int w, int h) { return getGraphData(w, h); };
    // const Component graphComp = Renderer([this] {return graph(func) | color(Color::BlueLight); });

    Component titlePane  = Container::Horizontal({ title | bold | border });
    Component statusPane = Container::Horizontal({ status | border | size(WIDTH, EQUAL, 32) });
    Component pane0      = Container::Horizontal({ titlePane, statusPane });
    Component pane1      = Container::Vertical({ dropDown | size(HEIGHT, EQUAL, 8) | size(WIDTH, EQUAL, mkDeviceSelectWidth) });
    Component pane2      = Container::Vertical({ openButton }) | border | size(HEIGHT, LESS_THAN, 16) | size(WIDTH, EQUAL, 10);
    Component meteringPane = Container::Horizontal({levelMeter});
    //Component graphPane  = Container::Vertical({ graphComp }) | border | size(WIDTH, EQUAL, 100);
    
    ButtonOption openCloseButtonStyle()
    {
        auto option      = ButtonOption::Animated(Color::White);

#pragma GCC diagnostic ignored "-Wunused-parameter"
        option.transform = [&](const EntryState &s){ return text(mDeviceIsOpen? "Close" : "Open") | size(HEIGHT, EQUAL, 1) | center | borderEmpty ;};
        return option;
    }

    void toggleDevice()
    {
        openButtonSignal.fire(mSelectedDevice, !mDeviceIsOpen);
    }

    std::string getDevDropdownLabel()
    {
        std::string label = "Device: " + (mDeviceNames.size() ? mDeviceNames.at(mSelectedDevice) : "");
        return trunc(label, mkDeviceSelectWidth - 4);
    }

    std::string trunc(const std::string& input, int maxLength)
    {
        if (input.length() <= static_cast<size_t>(maxLength)) {
            return input;
        } else {
            return input.substr(0, maxLength - 3) + "...";
        }
    }
};