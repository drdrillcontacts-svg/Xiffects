cmake_minimum_required(VERSION 3.22)
project(Xiffects VERSION 0.1.0)

include(CTest)
option(XIFFECTS_DSP_ONLY "Build portable DSP tests without JUCE/system GUI dependencies" OFF)
if(BUILD_TESTING)
    add_executable(XiffectsDSPTests Tests/DSPTests.cpp)
    target_include_directories(XiffectsDSPTests PRIVATE Source)
    target_compile_features(XiffectsDSPTests PRIVATE cxx_std_17)
    add_test(NAME DSP COMMAND XiffectsDSPTests)
    add_executable(XiffectsPitchTests Tests/PitchTests.cpp)
    target_include_directories(XiffectsPitchTests PRIVATE Source)
    target_compile_features(XiffectsPitchTests PRIVATE cxx_std_17)
    add_test(NAME Pitch COMMAND XiffectsPitchTests)
    find_package(Threads REQUIRED)
    add_executable(XiffectsRealtimeTests Tests/RealtimeTests.cpp)
    target_include_directories(XiffectsRealtimeTests PRIVATE Source)
    target_compile_features(XiffectsRealtimeTests PRIVATE cxx_std_17)
    target_link_libraries(XiffectsRealtimeTests PRIVATE Threads::Threads)
    add_test(NAME Realtime COMMAND XiffectsRealtimeTests)
endif()
if(XIFFECTS_DSP_ONLY)
    return()
endif()

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Xiffects
    COMPANY_NAME "Xivan"
    BUNDLE_ID "com.xivan.xiffects"
    PLUGIN_MANUFACTURER_CODE Xiva
    PLUGIN_CODE Xifx
    FORMATS VST3 Standalone
    PRODUCT_NAME "Xiffects"
    DESCRIPTION "Soprano Saxophone Multi-Effect"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_add_binary_data(XiffectsAssets SOURCES Assets/hero.jpg)
juce_generate_juce_header(Xiffects)
target_sources(Xiffects PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)
target_compile_features(Xiffects PUBLIC cxx_std_17)
target_compile_definitions(Xiffects PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_link_libraries(Xiffects PRIVATE
    XiffectsAssets
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

if(BUILD_TESTING)
    add_executable(XiffectsHostTests Tests/HostTests.cpp)
    target_compile_features(XiffectsHostTests PRIVATE cxx_std_17)
    target_include_directories(XiffectsHostTests PRIVATE Source
        $<TARGET_PROPERTY:Xiffects,INCLUDE_DIRECTORIES>)
    target_compile_definitions(XiffectsHostTests PRIVATE
        $<TARGET_PROPERTY:Xiffects,COMPILE_DEFINITIONS>)
    target_link_libraries(XiffectsHostTests PRIVATE Xiffects)
    add_test(NAME Host COMMAND XiffectsHostTests)
endif()
