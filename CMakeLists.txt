cmake_minimum_required(VERSION 3.22)
project(RBDrumSampler VERSION 0.1.0)

option(RBDS_BUILD_TESTS "Build unit tests" ON)

include(cmake/CPM.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)

if(RBDS_BUILD_TESTS)
    CPMAddPackage(
        NAME Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.4
    )
endif()

juce_add_plugin(RBDrumSampler
    PLUGIN_MANUFACTURER_CODE Vkry
    COMPANY_NAME "VKR"
    PLUGIN_CODE RBDs
    FORMATS VST3
    PRODUCT_NAME "Rock Band Drum Sampler"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(RBDrumSampler)

juce_add_binary_data(RBDrumSamplerData
    HEADER_NAME RBDrumSamplerData.h
    NAMESPACE BinaryData
    SOURCES
        ./audio/Kick1.ogg
        ./audio/Kick2.ogg
        ./audio/Snare1.ogg
        ./audio/HhClosed.ogg
        ./audio/Ride1.ogg
        ./audio/Crsh1.ogg
        ./audio/Crsh2.ogg
        ./audio/HiMidTom.ogg
        ./audio/LoMidTom.ogg
        ./audio/LowFlrTom.ogg
)

if (MSVC)
    target_compile_options(RBDrumSamplerData PRIVATE /bigobj)
endif()

target_sources(RBDrumSampler
    PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
)

target_compile_definitions(RBDrumSampler
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(RBDrumSampler
    PRIVATE
        RBDrumSamplerData
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(RBDS_BUILD_TESTS)
    juce_add_console_app(RBDrumSamplerTests PRODUCT_NAME "RBDrumSamplerTests")
    juce_generate_juce_header(RBDrumSamplerTests)

    target_sources(RBDrumSamplerTests PRIVATE
        src/PluginProcessor.cpp
        src/PluginEditor.cpp
        tests/TestMain.cpp
        tests/MidiRemapTests.cpp
        tests/AudioSmokeTests.cpp
        tests/StateTests.cpp)

    target_compile_definitions(RBDrumSamplerTests
        PRIVATE
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_VST3_CAN_REPLACE_VST2=0
            JucePlugin_Name="Rock Band Drum Sampler")

    target_link_libraries(RBDrumSamplerTests
        PRIVATE
            RBDrumSamplerData
            Catch2::Catch2
            juce::juce_audio_processors
            juce::juce_audio_formats
            juce::juce_audio_utils
        PUBLIC
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags)
endif()
