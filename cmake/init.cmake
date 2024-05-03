macro(init)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

    include(FetchContent)
    set(FETCHCONTENT_QUIET OFF)

    include(portaudio)
    include(spdlog)
    include(ftxui)
    include(nanosigslot)

endmacro(init)