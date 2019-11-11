cmake_minimum_required(VERSION 3.15)
project(Phase_3)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(phase3a)

add_executable(Phase_3
        phase3a/p3/p3stubs.c
        phase3a/p3/p3stubsTest.c
        phase3a/tests/test_basic.c
        phase3a/Makefile
        phase3a/phase3a.c
        CS452_Fall_2019_Phase_3.pdf
        Makefile
        phase3.h
        phase3Int.h
        README.md
        subdir.mk
        tester.h
        phase2.h
        versions.mk)
