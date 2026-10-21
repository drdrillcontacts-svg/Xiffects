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
    add_executable(XiffectsOnsetTests Tests/OnsetTests.cpp)
    target_include_directories(XiffectsOnsetTests PRIVATE Source)
    target_compile_features(XiffectsOnsetTests PRIVATE cxx_std_17)
    add_test(NAME Onset COMMAND XiffectsOnsetTests)
    add_executable(XiffectsLivePitchTests Tests/LivePitchTests.cpp)
    target_include_directories(XiffectsLivePitchTests PRIVATE Source)
    target_compile_features(XiffectsLivePitchTests PRIVATE cxx_std_17)
    add_test(NAME LivePitch COMMAND XiffectsLivePitchTests)
    add_executable(XiffectsCorrectionRegressionTests Tests/CorrectionRegressionTests.cpp)
    target_include_directories(XiffectsCorrectionRegressionTests PRIVATE Source)
    target_compile_features(XiffectsCorrectionRegressionTests PRIVATE cxx_std_17)
    add_test(NAME CorrectionRegression COMMAND XiffectsCorrectionRegressionTests)
    find_package(Threads REQUIRED)
    add_executable(XiffectsRealtimeTests Tests/RealtimeTests.cpp)
    target_include_directories(XiffectsRealtimeTests PRIVATE Source)
    target_compile_features(XiffectsRealtimeTests PRIVATE cxx_std_17)
    target_link_libraries(XiffectsRealtimeTests PRIVATE Threads::Threads)
    add_test(NAME Realtime COMMAND XiffectsRealtimeTests)
endif()
option(XIFFECTS_BUILD_BENCHMARKS "Build offline pitch-path measurement tool (not a live-input test)" OFF)
if(XIFFECTS_BUILD_BENCHMARKS)
    add_executable(XiffectsPitchProfile Tools/PitchProfile.cpp)
    target_include_directories(XiffectsPitchProfile PRIVATE Source)
    target_compile_features(XiffectsPitchProfile PRIVATE cxx_std_17)
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

# Optional offline harness. Links the same product library, never a DSP copy.
option(XIFFECTS_BUILD_REFERENCE_TOOLS "Build real-processor offline Reference renderer" OFF)
if(XIFFECTS_BUILD_REFERENCE_TOOLS)
    add_executable(XiffectsReferenceRender Tools/ReferenceRender.cpp)
    target_compile_features(XiffectsReferenceRender PRIVATE cxx_std_17)
    target_include_directories(XiffectsReferenceRender PRIVATE Source
        $<TARGET_PROPERTY:Xiffects,INCLUDE_DIRECTORIES>)
    target_compile_definitions(XiffectsReferenceRender PRIVATE
        $<TARGET_PROPERTY:Xiffects,COMPILE_DEFINITIONS>)
    target_link_libraries(XiffectsReferenceRender PRIVATE Xiffects)
    if(BUILD_TESTING)
        add_executable(XiffectsReferenceTests Tests/ReferenceTests.cpp)
        target_compile_features(XiffectsReferenceTests PRIVATE cxx_std_17)
        target_include_directories(XiffectsReferenceTests PRIVATE Source
            $<TARGET_PROPERTY:Xiffects,INCLUDE_DIRECTORIES>)
        target_compile_definitions(XiffectsReferenceTests PRIVATE
            $<TARGET_PROPERTY:Xiffects,COMPILE_DEFINITIONS>)
        target_link_libraries(XiffectsReferenceTests PRIVATE Xiffects)
        add_test(NAME ReferenceProcessor COMMAND XiffectsReferenceTests)
    endif()
endif()

# Separate product ID: the host never has to change latency mid-performance and
# old sessions keep the original Studio processing path.
option(XIFFECTS_BUILD_LIVE "Build experimental low-latency Live VST3 and standalone" OFF)
if(XIFFECTS_BUILD_LIVE)
    juce_add_plugin(XiffectsLive
        COMPANY_NAME "Xivan" BUNDLE_ID "com.xivan.xiffects.live"
        PLUGIN_MANUFACTURER_CODE Xiva PLUGIN_CODE Xifl
        FORMATS VST3 Standalone PRODUCT_NAME "Xiffects Live"
        DESCRIPTION "Experimental low-latency soprano saxophone multi-effect"
        IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE COPY_PLUGIN_AFTER_BUILD FALSE)
    juce_generate_juce_header(XiffectsLive)
    target_sources(XiffectsLive PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_features(XiffectsLive PUBLIC cxx_std_17)
    target_compile_definitions(XiffectsLive PUBLIC XIFFECTS_LIVE_MODE=1
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_link_libraries(XiffectsLive PRIVATE XiffectsAssets juce::juce_audio_utils
        juce::juce_audio_processors juce::juce_dsp PUBLIC
        juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
    foreach(tool LiveHostTests LiveRender)
        if(tool STREQUAL "LiveHostTests")
            set(tool_source Tests/LiveHostTests.cpp)
        else()
            set(tool_source Tools/ReferenceRender.cpp)
        endif()
        add_executable(Xiffects${tool} ${tool_source})
        target_compile_features(Xiffects${tool} PRIVATE cxx_std_17)
        target_include_directories(Xiffects${tool} PRIVATE Source $<TARGET_PROPERTY:XiffectsLive,INCLUDE_DIRECTORIES>)
        target_compile_definitions(Xiffects${tool} PRIVATE $<TARGET_PROPERTY:XiffectsLive,COMPILE_DEFINITIONS>)
        target_link_libraries(Xiffects${tool} PRIVATE XiffectsLive)
    endforeach()
    if(BUILD_TESTING)
        add_test(NAME LiveHost COMMAND XiffectsLiveHostTests)
    endif()
endif()
