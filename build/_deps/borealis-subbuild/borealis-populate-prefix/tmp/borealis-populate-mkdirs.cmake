# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/mohan/clawd/switchby/borealis"
  "/home/mohan/clawd/switchby/build/_deps/borealis-build"
  "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix"
  "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/tmp"
  "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/src/borealis-populate-stamp"
  "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/src"
  "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/src/borealis-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/src/borealis-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/mohan/clawd/switchby/build/_deps/borealis-subbuild/borealis-populate-prefix/src/borealis-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
