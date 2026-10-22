cmake_minimum_required(VERSION 3.10)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(RearVitaKillerSwitch C)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "RearVitaKillerSwitch")
set(VITA_VERSION "01.00")

add_executable(${PROJECT_NAME}
  main.c
)

target_link_libraries(${PROJECT_NAME}
  taihen_stub
  SceTouch_stub
  SceLibKernel_stub
)

set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-nostdlib")

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${CMAKE_SOURCE_DIR}/exports.yml
)
