cmake_minimum_required(VERSION 3.22)

project(PanLoop VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Keep JUCE's own examples/extras out of the build.
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.15
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(pan-loop
    PRODUCT_NAME "Pan Loop"
    COMPANY_NAME "Pan Loop"
    BUNDLE_ID "dev.panloop.app"
    VERSION ${PROJECT_VERSION}
    NEEDS_CURL FALSE
    NEEDS_WEB_BROWSER FALSE
)

juce_generate_juce_header(pan-loop)

target_include_directories(pan-loop PRIVATE src)

target_sources(pan-loop PRIVATE
    src/Main.cpp
    src/app/DarkLookAndFeel.cpp
    src/app/DarkLookAndFeel.h
    src/app/MainComponent.cpp
    src/app/MainComponent.h
    src/audio/AudioEngine.cpp
    src/audio/AudioEngine.h
    src/audio/AudioSettingsPanel.cpp
    src/audio/AudioSettingsPanel.h
    src/audio/AudioSetup.cpp
    src/audio/AudioSetup.h
    src/core/AppConfig.cpp
    src/core/AppConfig.h
    src/midi/MidiManager.cpp
    src/midi/MidiManager.h
    src/midi/MidiPanel.cpp
    src/midi/MidiPanel.h
    src/ui/LevelMeter.cpp
    src/ui/LevelMeter.h
)

target_compile_definitions(pan-loop PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:pan-loop,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="${PROJECT_VERSION}"
    JUCE_ALSA=1
    JUCE_JACK=1
    JUCE_JACK_CLIENT_NAME=\"pan-loop\"
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_USE_OGGVORBIS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_REPORT_APP_USAGE=0
)

target_link_libraries(pan-loop PRIVATE
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
