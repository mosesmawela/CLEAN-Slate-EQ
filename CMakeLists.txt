cmake_minimum_required(VERSION 3.22)

project(CLEAN_SLATE_EQ VERSION 0.0.1)

# Enable FetchContent
include(FetchContent)

# Set FetchContent JUCE
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        origin/master # You can pin this to a specific version like 8.0.0
)

# Optional: Speed up FetchContent by only checking once
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(juce)

# Create the plugin target
juce_add_plugin(CLEAN_Slate_EQ
    VERSION 0.0.1
    COMPANY_NAME "MosesMawela"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_CODE "ClSq"
    FORMATS VST3 Standalone
    PRODUCT_NAME "CLEAN Slate EQ"
    JUCE_MANUFACTURER_CODE "ClSt"
    BUNDLE_ID "com.mosesmawela.cleanslateeq"
    VST2_CATEGORY Vst2Category::kVST2CategoryEffect
)

# Add source files
target_sources(CLEAN_Slate_EQ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Presets.h
)

# Link to JUCE modules
target_link_libraries(CLEAN_Slate_EQ PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_extra
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_core
    juce::juce_opengl
)

# C++ Standard
target_compile_definitions(CLEAN_Slate_EQ PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)
target_compile_features(CLEAN_Slate_EQ PRIVATE cxx_std_17)

# Position independent code for VST3
set_target_properties(CLEAN_Slate_EQ PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
