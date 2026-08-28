cmake_minimum_required(VERSION 3.22)

project(Livellatore VERSION 0.6.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# AU e' un formato solo Apple: fuori da macOS juce_add_plugin fallirebbe a
# generare quel target (issue #7 - preparazione per build Windows/Linux,
# mai compilate/verificate perche' non c'e' un Mac/Windows/Linux dove
# farlo tutti insieme in questa sessione).
set(LIVELLATORE_FORMATS VST3 Standalone)
if (APPLE)
    list(APPEND LIVELLATORE_FORMATS AU)
endif()

juce_add_plugin(Livellatore
    COMPANY_NAME "gas"
    PLUGIN_MANUFACTURER_CODE Gasx
    PLUGIN_CODE Lvlr
    FORMATS ${LIVELLATORE_FORMATS}
    PRODUCT_NAME "Livellatore"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(Livellatore
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PresetManager.cpp
        Source/dsp/LoudnessMeter.cpp
        Source/dsp/GainRider.cpp
        Source/dsp/Limiter.cpp
        Source/dsp/LevelerEngine.cpp
        Source/ui/LivellatoreLookAndFeel.cpp
        Source/ui/VuMeterComponent.cpp
        Source/ui/LevelSliderComponent.cpp
)

target_include_directories(Livellatore
    PRIVATE
        Source
)

target_compile_definitions(Livellatore
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Livellatore
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

enable_testing()
add_subdirectory(Tests)
