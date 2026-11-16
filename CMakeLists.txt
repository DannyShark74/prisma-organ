cmake_minimum_required(VERSION 3.22)
project(Prisma VERSION 0.1.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PRISMA_BUILD_TOOLS "Build the offline render / test tool" OFF)
option(PRISMA_BUILD_PLUGIN "Build the plugin" ON)
set(PRISMA_JUCE_DIR "" CACHE PATH "Local JUCE checkout. If empty, JUCE is downloaded.")

if(PRISMA_JUCE_DIR)
    add_subdirectory(${PRISMA_JUCE_DIR} juce EXCLUDE_FROM_ALL)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.9
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

if(APPLE)
    set(PRISMA_FORMATS VST3 AU)
else()
    set(PRISMA_FORMATS VST3)
endif()

set(PRISMA_DSP_SOURCES
    source/Tonewheel.cpp
    source/Combo.cpp
    source/Effects.cpp
    source/Engine.cpp
    source/ParamTable.cpp
    source/Presets.cpp)

if(PRISMA_BUILD_PLUGIN)
    juce_add_plugin(Prisma
        PRODUCT_NAME "Prisma"
        VERSION ${PROJECT_VERSION}
        COMPANY_NAME "Danny Shark"
        BUNDLE_ID com.dannyshark.prisma
        PLUGIN_MANUFACTURER_CODE Dnsh
        PLUGIN_CODE Prsm
        PLUGIN_NAME "Prisma"
        DESCRIPTION "Psychedelic organ: tonewheel + combo, rotary, drum echo"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Instrument Synth
        AU_MAIN_TYPE kAudioUnitType_MusicDevice
        FORMATS ${PRISMA_FORMATS})

    target_sources(Prisma PRIVATE
        ${PRISMA_DSP_SOURCES}
        source/Params.cpp
        source/PluginProcessor.cpp
        source/PluginEditor.cpp)
    target_include_directories(Prisma PRIVATE source)
    target_compile_definitions(Prisma PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0)
    target_link_libraries(Prisma
        PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
endif()

if(PRISMA_BUILD_TOOLS)
    juce_add_console_app(PrismaRender PRODUCT_NAME "PrismaRender")
    target_sources(PrismaRender PRIVATE tools/render.cpp ${PRISMA_DSP_SOURCES})
    target_include_directories(PrismaRender PRIVATE source)
    target_compile_definitions(PrismaRender PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)
    target_link_libraries(PrismaRender PRIVATE juce::juce_audio_formats juce::juce_audio_processors juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()

if(PRISMA_BUILD_TOOLS AND PRISMA_BUILD_PLUGIN)
    # renders the editor to a PNG (layout check without a DAW)
    juce_add_console_app(PrismaShot PRODUCT_NAME "PrismaShot")
    target_sources(PrismaShot PRIVATE tools/shot.cpp ${PRISMA_DSP_SOURCES}
        source/Params.cpp source/PluginProcessor.cpp source/PluginEditor.cpp)
    target_include_directories(PrismaShot PRIVATE source)
    target_compile_definitions(PrismaShot PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0 JucePlugin_Name="Prisma")
    target_link_libraries(PrismaShot PRIVATE juce::juce_audio_utils juce::juce_dsp
        PUBLIC juce::juce_recommended_config_flags)
endif()
