# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "E:/Repositories/EngineForAnimationCourse/cmake/../external/libigl-imgui"
  "E:/Repositories/Animation_igl_engine_build_assignment3.5/bin/libigl-imgui-build"
  "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix"
  "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix/tmp"
  "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp"
  "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src"
  "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp"
)

set(configSubDirs Debug;Release;MinSizeRel;RelWithDebInfo)
foreach(subDir IN LISTS configSubDirs)
  file(MAKE_DIRECTORY "E:/Repositories/EngineForAnimationCourse/external/.cache/libigl-imgui/libigl-imgui-download-prefix/src/libigl-imgui-download-stamp/${subDir}")
endforeach()
