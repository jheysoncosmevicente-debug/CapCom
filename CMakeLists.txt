cmake_minimum_required(VERSION 3.22)

# Mac: un solo binario universal (Apple Silicon + Intel), desde macOS 11 (Big Sur).
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Arquitecturas de Mac")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS minimo")
endif()

project(CapCom VERSION 1.0.0)

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# JUCE 8.0.10 se descarga solo. Para usar una copia local: -DFETCHCONTENT_SOURCE_DIR_JUCE=/ruta/a/JUCE
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.10
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Mac: AU + VST3 + app. Windows y Linux: VST3 + app.
if(APPLE)
    set(CAPCOM_FORMATS AU VST3 Standalone)
else()
    set(CAPCOM_FORMATS VST3 Standalone)
endif()

# Copiar el plugin a la carpeta de plugins del usuario al compilar (solo Mac; en Windows esa carpeta pide permisos de administrador).
option(CAPCOM_INSTALL_AFTER_BUILD "Copiar el AU y el VST3 a ~/Library/Audio/Plug-Ins al compilar" ${APPLE})

juce_add_plugin(CapCom
    COMPANY_NAME "Jhey"
    BUNDLE_ID "com.jhey.capcom"
    # Codigos del plugin: DaVinci y Premiere lo identifican por ellos y los proyectos guardados dependen de eso. No cambiarlos.
    PLUGIN_MANUFACTURER_CODE Jhey
    PLUGIN_CODE Cpcm
    FORMATS ${CAPCOM_FORMATS}
    PRODUCT_NAME "CapCom"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    AU_MAIN_TYPE kAudioUnitType_Effect
    VST3_CATEGORIES Fx Distortion Filter
    COPY_PLUGIN_AFTER_BUILD ${CAPCOM_INSTALL_AFTER_BUILD})

# Logotipo de la cabecera (PNG con transparencia, al doble de tamano para pantallas Retina)
juce_add_binary_data(CapComAssets SOURCES Assets/capcom-logo.png)

set(CAPCOM_SOURCES
    Source/RadioEngine.cpp
    Source/SceneEngine.cpp
    Source/RadioChain.cpp
    Source/EqEditor.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_sources(CapCom PRIVATE ${CAPCOM_SOURCES})
target_compile_features(CapCom PRIVATE cxx_std_17)

target_compile_definitions(CapCom PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# Los textos con tildes estan en UTF-8
if(MSVC)
    target_compile_options(CapCom PRIVATE /utf-8)
endif()

target_link_libraries(CapCom
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_animation
        CapComAssets
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Herramienta para desarrollo: dibuja la interfaz a PNG y exporta audio con un preset
# (cmake --build build --target CapComSnapshot)
juce_add_console_app(CapComSnapshot PRODUCT_NAME "CapComSnapshot")
set_target_properties(CapComSnapshot PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(CapComSnapshot PRIVATE tools/Snapshot.cpp ${CAPCOM_SOURCES})
target_compile_features(CapComSnapshot PRIVATE cxx_std_17)
target_compile_definitions(CapComSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
if(MSVC)
    target_compile_options(CapComSnapshot PRIVATE /utf-8)
endif()
target_link_libraries(CapComSnapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_animation CapComAssets juce::juce_recommended_config_flags)
