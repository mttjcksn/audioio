FetchContent_Declare(
    portaudio
    GIT_REPOSITORY      https://github.com/PortAudio/portaudio
    GIT_TAG             master
    GIT_PROGRESS        ON
    UPDATE_DISCONNECTED ON
    SYSTEM
)
FetchContent_MakeAvailable(portaudio)
