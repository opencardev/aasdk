#include <cstring>
#include <benchmark/benchmark.h>

#include <aasdk/Common/Data.hpp>

using namespace aasdk::common;

// -- DataConstBuffer construction benchmarks --

static void BM_DataConstBufferFromVector(benchmark::State &state) {
  Data data(state.range(0), 0x42);

  for (auto _ : state) {
    DataConstBuffer buffer(data);
    benchmark::DoNotOptimize(buffer.cdata);
    benchmark::DoNotOptimize(buffer.size);
  }
}
BENCHMARK(BM_DataConstBufferFromVector)->Range(64, 65536);

static void BM_DataConstBufferWithOffset(benchmark::State &state) {
  Data data(4096, 0x42);

  for (auto _ : state) {
    DataConstBuffer buffer(data, 128);
    benchmark::DoNotOptimize(buffer.cdata);
    benchmark::DoNotOptimize(buffer.size);
  }
}
BENCHMARK(BM_DataConstBufferWithOffset);

// -- Data copy benchmarks --

static void BM_DataCopySmall(benchmark::State &state) {
  Data source(64, 0xAB);
  DataConstBuffer buffer(source);

  for (auto _ : state) {
    Data dest;
    copy(dest, buffer);
    benchmark::DoNotOptimize(dest);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_DataCopySmall);

static void BM_DataCopy(benchmark::State &state) {
  Data source(state.range(0), 0xAB);
  DataConstBuffer buffer(source);

  for (auto _ : state) {
    Data dest;
    copy(dest, buffer);
    benchmark::DoNotOptimize(dest);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_DataCopy)->Range(256, 1 << 20);

static void BM_DataCopyAppend(benchmark::State &state) {
  Data source(1024, 0xCD);
  DataConstBuffer buffer(source);

  for (auto _ : state) {
    Data dest;
    dest.reserve(4096);
    for (int i = 0; i < 4; ++i) {
      copy(dest, buffer);
    }
    benchmark::DoNotOptimize(dest);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_DataCopyAppend);

// -- createData benchmark --

static void BM_CreateData(benchmark::State &state) {
  Data source(state.range(0), 0xEF);
  DataConstBuffer buffer(source);

  for (auto _ : state) {
    Data result = createData(buffer);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_CreateData)->Range(64, 65536);

// -- Data dump (hex encoding) benchmarks --

static void BM_DataDump(benchmark::State &state) {
  Data data(state.range(0), 0xAB);

  for (auto _ : state) {
    std::string result = dump(data);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_DataDump)->Range(16, 4096);

static void BM_DataDumpBuffer(benchmark::State &state) {
  Data data(state.range(0), 0xCD);
  DataConstBuffer buffer(data);

  for (auto _ : state) {
    std::string result = dump(buffer);
    benchmark::DoNotOptimize(result);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_DataDumpBuffer)->Range(16, 4096);

// -- DataBuffer equality benchmarks --

static void BM_DataConstBufferEquality(benchmark::State &state) {
  Data data(1024, 0x42);
  DataConstBuffer buf1(data);
  DataConstBuffer buf2(data);

  for (auto _ : state) {
    bool result = (buf1 == buf2);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_DataConstBufferEquality);

static void BM_DataConstBufferNullCheck(benchmark::State &state) {
  DataConstBuffer buffer;

  for (auto _ : state) {
    bool result = (buffer == nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_DataConstBufferNullCheck);
