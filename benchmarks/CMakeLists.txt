cmake_minimum_required(VERSION 3.16)

include(FetchContent)

set(BENCHMARK_DOWNLOAD_DEPENDENCIES ON)

FetchContent_Declare(
    google_benchmark
    GIT_REPOSITORY https://github.com/CodSpeedHQ/codspeed-cpp
    SOURCE_SUBDIR google_benchmark
    GIT_TAG main
)

FetchContent_MakeAvailable(google_benchmark)

add_executable(aasdk_benchmarks
    bench_messenger.cpp
    bench_data.cpp
)

target_link_libraries(aasdk_benchmarks
    PRIVATE
        benchmark::benchmark
        aasdk
)

target_include_directories(aasdk_benchmarks
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_BINARY_DIR}/include
)
