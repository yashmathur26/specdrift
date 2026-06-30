# SPECDRIFT - Spectral Blur Effect Plugin
# JUCE 7+ | VST3, AU | macOS, Windows

cmake_minimum_required(VERSION 3.22)

project(Specdrift VERSION 1.0.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)

# JUCE path: sibling directory or override with -DJUCE_PATH=/path/to/JUCE
set(JUCE_PATH "${CMAKE_SOURCE_DIR}/../JUCE" CACHE PATH "Path to JUCE")
if(NOT EXISTS "${JUCE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR "JUCE not found at ${JUCE_PATH}. Clone JUCE as a sibling folder or pass -DJUCE_PATH=/path/to/JUCE")
endif()

add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

# Plugin target (pure C++ UI, no WebView)
juce_add_plugin(Specdrift
    PLUGIN_MANUFACTURER_CODE Spec
    PLUGIN_CODE SpD1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Specdrift"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    NEEDS_WEB_BROWSER FALSE)

target_sources(Specdrift
    PRIVATE
        Source/Core/PluginProcessor.cpp
        Source/Core/PluginEditor.cpp
        Source/Core/Parameters.cpp
        Source/DSP/CurveState.cpp
        Source/DSP/SpectralEngine.cpp
        Source/UI/OrbKnobComponent.cpp
        Source/UI/FooterSliderComponent.cpp
        Source/UI/SpectrogramComponent.cpp)

target_include_directories(Specdrift
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Source
        ${CMAKE_SOURCE_DIR}/Source/Core
        ${CMAKE_SOURCE_DIR}/Source/DSP
        ${CMAKE_SOURCE_DIR}/Source/UI)

target_compile_definitions(Specdrift
    PUBLIC
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Specdrift
    PRIVATE
        juce::juce_audio_utils
        juce::juce_gui_extra
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
