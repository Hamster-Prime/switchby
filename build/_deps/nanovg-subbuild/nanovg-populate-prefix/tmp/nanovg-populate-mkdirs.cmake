# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/mohan/clawd/switchby/nanovg"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-build"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/tmp"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/src/nanovg-populate-stamp"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/src"
  "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/src/nanovg-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/src/nanovg-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/mohan/clawd/switchby/build/_deps/nanovg-subbuild/nanovg-populate-prefix/src/nanovg-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
