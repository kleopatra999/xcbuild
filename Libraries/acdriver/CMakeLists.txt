#
# Copyright (c) 2015-present, Facebook, Inc.
# All rights reserved.
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree. An additional grant
# of patent rights can be found in the PATENTS file in the same directory.
#

add_library(acdriver SHARED
            Sources/Options.cpp
            Sources/Driver.cpp
            Sources/Output.cpp
            Sources/Result.cpp
            Sources/VersionAction.cpp
            Sources/CompileAction.cpp
            Sources/ContentsAction.cpp
            )

target_link_libraries(acdriver PUBLIC xcassets util plist ext)
target_include_directories(acdriver PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Headers")
install(TARGETS acdriver DESTINATION usr/lib)

add_executable(actool Tools/actool.cpp)
target_link_libraries(actool PRIVATE acdriver)
install(TARGETS actool DESTINATION usr/bin)

if (BUILD_TESTING)
  ADD_UNIT_GTEST(acdriver Options Tests/test_Options.cpp)
  ADD_UNIT_GTEST(acdriver Output Tests/test_Output.cpp)
  ADD_UNIT_GTEST(acdriver Result Tests/test_Result.cpp)
endif ()
