# Define the version and URL of nano-signal-slot
set(NANO_SIGNAL_SLOT_VERSION "2.0.1")
set(NANO_SIGNAL_SLOT_URL "https://github.com/NoAvailableAlias/nano-signal-slot/archive/v${NANO_SIGNAL_SLOT_VERSION}.zip")

# Fetch the nano-signal-slot library
FetchContent_Declare(
    nano_signal_slot
    URL ${NANO_SIGNAL_SLOT_URL}
)

FetchContent_MakeAvailable(nano_signal_slot)