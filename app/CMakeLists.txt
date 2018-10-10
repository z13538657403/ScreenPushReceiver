cmake_minimum_required(VERSION 3.4.1)

set(lib_DIR ${CMAKE_SOURCE_DIR}/src/main/jniLibs)

add_library(jthread STATIC IMPORTED)
set_target_properties(jthread PROPERTIES IMPORTED_LOCATION
      ${lib_DIR}/${ANDROID_ABI}/libjthread.a)

add_library(jrtplib STATIC IMPORTED)
set_target_properties(jrtplib PROPERTIES IMPORTED_LOCATION
      ${lib_DIR}/${ANDROID_ABI}/libjrtplib.a)

include_directories(${lib_DIR}/include)

add_library(native-lib
             SHARED
             src/main/cpp/native-lib.cpp)

find_library(log-lib
              log )

target_link_libraries(native-lib
                      jrtplib
                      jthread
                       ${log-lib})
