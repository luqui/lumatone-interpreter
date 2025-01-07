cmake_minimum_required(VERSION 3.28.4)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(LUMATONE_INTERPRETER VERSION 0.0.1)

include(cmake/CPM.cmake)

CPMAddPackage(
    GITHUB_REPOSITORY "juce-framework/JUCE"
    GIT_TAG "8.0.4"
    OPTIONS "JUCE_ENABLE_MODULE_SOURCE_GROUPS ON"
)

# `juce_add_plugin` adds a static library target with the name passed as the first argument.
# This target is a normal CMake target, but has a lot of extra properties set
# up by default. As well as this shared code static library, this function adds targets for each of
# the formats specified by the FORMATS arguments. This function accepts many optional arguments.
# Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.
set(PLUGIN_TARGET LumatoneInterpreter)
juce_add_plugin(${PLUGIN_TARGET}
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    COMPANY_NAME "Luke Palmer"
    COMPANY_WEBSITE "https://luqui.github.io"
    COMPANY_EMAIL "lrpalmer@gmail.com"
    BUNDLE_ID "io.github.luqui.lumatone-interpreter"
    DESCRIPTION "Lumatone Controller"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT TRUE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    STATUS_BAR_HIDDEN TRUE
    FILE_SHARING_ENABLED TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Lqui               # A four-character manufacturer id with at least one upper-case character
    PLUGIN_CODE Ltct                            # A unique four-character plugin id with exactly one upper-case character
                                                # GarageBand 10.3 requires the first letter to be upper-case, and the remaining letters to be lower-case
    FORMATS Standalone;VST3;AUv3
    PRODUCT_NAME "Lumatone Interpreter")                    # The name of the final executable, which can differ from the target name

target_include_directories(${PLUGIN_TARGET}
    PRIVATE
        Source
)

set(shared_sources
    Source/Plugin.h
    Source/Plugin.cpp
)

set(standalone_sources
    Source/Main.cpp
)

target_sources(${PLUGIN_TARGET}
    PRIVATE
        ${shared_sources}
)

target_sources(${PLUGIN_TARGET}_Standalone
    PRIVATE
        ${standalone_sources}
)

target_compile_definitions(${PLUGIN_TARGET}
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_STRICT_REFCOUNTEDPOINTER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        _USE_MATH_DEFINES=1
)

target_compile_definitions(${PLUGIN_TARGET}_Standalone
    PRIVATE
        JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1
)

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

target_link_libraries(${PLUGIN_TARGET}
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_plugin_client
        juce::juce_audio_utils
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        -Werror=return-type
)
