cmake_minimum_required(VERSION 3.22)

project(PanLoop VERSION 0.1.0 LANGUAGES C CXX)

include(GNUInstallDirs)

if (UNIX AND NOT APPLE)
    find_package(ALSA REQUIRED)
endif()

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

# A space-free executable name keeps desktop entries, AppImage launchers and
# shell invocations simple on Linux. The window title remains "Pan Loop".
if (UNIX AND NOT APPLE)
    set_target_properties(pan-loop PROPERTIES OUTPUT_NAME "pan-loop")
endif()

target_include_directories(pan-loop PRIVATE src)

target_sources(pan-loop PRIVATE
    src/Main.cpp
    src/app/DarkLookAndFeel.cpp
    src/app/DarkLookAndFeel.h
    src/app/LayersPanel.cpp
    src/app/LayersPanel.h
    src/app/LivePanel.cpp
    src/app/LivePanel.h
    src/app/MainComponent.cpp
    src/app/MainComponent.h
    src/app/PerformancePanel.cpp
    src/app/PerformancePanel.h
    src/app/PlaylistPanel.cpp
    src/app/PlaylistPanel.h
    src/audio/AudioEngine.cpp
    src/audio/AudioEngine.h
    src/audio/AudioFileLoader.cpp
    src/audio/AudioFileLoader.h
    src/audio/AudioSettingsPanel.cpp
    src/audio/AudioSettingsPanel.h
    src/audio/AudioSetup.cpp
    src/audio/BufferSizeControl.cpp
    src/audio/BufferSizeControl.h
    src/audio/AudioSetup.h
    src/audio/LiveLooper.cpp
    src/audio/LiveLooper.h
    src/audio/Metronome.cpp
    src/audio/Metronome.h
    src/audio/Playlist.cpp
    src/audio/Playlist.h
    src/audio/Transport.cpp
    src/audio/Transport.h
    src/core/AppConfig.cpp
    src/core/AppConfig.h
    src/midi/MidiAction.h
    src/midi/MidiManager.cpp
    src/midi/MidiManager.h
    src/midi/MidiMapping.h
    src/midi/MidiPanel.cpp
    src/midi/MidiPanel.h
    src/midi/RawMidiInput.cpp
    src/midi/RawMidiInput.h
    src/session/SessionStore.cpp
    src/session/SessionStore.h
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

if (UNIX AND NOT APPLE)
    target_link_libraries(pan-loop PRIVATE ${ALSA_LIBRARIES})
endif()

option(PANLOOP_BUILD_TESTS "Build the Pan Loop test suite" ON)

if (PANLOOP_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(pan-loop-tests
        PRODUCT_NAME "Pan Loop Tests"
        NEEDS_CURL FALSE
        NEEDS_WEB_BROWSER FALSE
    )

    juce_generate_juce_header(pan-loop-tests)

    target_include_directories(pan-loop-tests PRIVATE src)

    target_sources(pan-loop-tests PRIVATE
        tests/TestMain.cpp
        src/audio/AudioEngine.cpp
        src/audio/AudioEngine.h
        src/audio/AudioFileLoader.cpp
        src/audio/AudioFileLoader.h
        src/audio/BufferSizeControl.cpp
        src/audio/BufferSizeControl.h
        src/audio/LiveLooper.cpp
        src/audio/LiveLooper.h
        src/audio/Metronome.cpp
        src/audio/Metronome.h
        src/audio/Playlist.cpp
        src/audio/Playlist.h
        src/audio/Transport.cpp
        src/audio/Transport.h
        src/midi/MidiAction.h
        src/midi/MidiMapping.h
        src/midi/RawMidiInput.cpp
        src/midi/RawMidiInput.h
        src/session/SessionStore.cpp
        src/session/SessionStore.h
    )

    target_compile_definitions(pan-loop-tests PRIVATE
        JUCE_ALSA=1
        JUCE_USE_MP3AUDIOFORMAT=1
        JUCE_USE_OGGVORBIS=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_REPORT_APP_USAGE=0
    )

    target_link_libraries(pan-loop-tests PRIVATE
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
    )

    if (UNIX AND NOT APPLE)
        target_link_libraries(pan-loop-tests PRIVATE ${ALSA_LIBRARIES})
    endif()

    add_test(NAME pan-loop-tests COMMAND pan-loop-tests)
endif()

install(TARGETS pan-loop RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES packaging/pan-loop.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/pan-loop.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/pan-loop)
