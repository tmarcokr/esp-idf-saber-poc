cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS 
    "components/audio/audio_channel"
    "components/audio/audio_engine"
    "components/audio/i2s_transmitter"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(saber-poc)
