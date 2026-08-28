add_executable(LivellatoreTests
    main.cpp
    DspTests.cpp
    PresetManagerTests.cpp
    ${CMAKE_SOURCE_DIR}/Source/dsp/LoudnessMeter.cpp
    ${CMAKE_SOURCE_DIR}/Source/dsp/GainRider.cpp
    ${CMAKE_SOURCE_DIR}/Source/dsp/Limiter.cpp
    ${CMAKE_SOURCE_DIR}/Source/dsp/LevelerEngine.cpp
    ${CMAKE_SOURCE_DIR}/Source/PresetManager.cpp
)

target_include_directories(LivellatoreTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)

target_link_libraries(LivellatoreTests
    PRIVATE
        juce::juce_audio_basics
        juce::juce_data_structures
        juce::juce_graphics
        juce::juce_dsp
        juce::juce_recommended_config_flags
)

target_compile_definitions(LivellatoreTests PRIVATE JUCE_STANDALONE_APPLICATION=1)

add_test(NAME LivellatoreDspTests COMMAND LivellatoreTests)
