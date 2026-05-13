#include <cstring>
#include <benchmark/benchmark.h>

#include <aasdk/Common/Data.hpp>
#include <aasdk/Messenger/FrameHeader.hpp>
#include <aasdk/Messenger/FrameSize.hpp>
#include <aasdk/Messenger/MessageId.hpp>
#include <aasdk/Messenger/Message.hpp>

using namespace aasdk::messenger;
using namespace aasdk::common;

// -- FrameHeader benchmarks --

static void BM_FrameHeaderParse(benchmark::State &state) {
  // Simulate a raw 2-byte frame header: channel=CONTROL, type=BULK|PLAIN|SPECIFIC
  uint8_t raw[2] = {
      static_cast<uint8_t>(ChannelId::CONTROL),
      static_cast<uint8_t>(FrameType::BULK) |
          static_cast<uint8_t>(EncryptionType::PLAIN) |
          static_cast<uint8_t>(MessageType::SPECIFIC)};
  DataConstBuffer buffer(raw, sizeof(raw));

  for (auto _ : state) {
    FrameHeader header(buffer);
    benchmark::DoNotOptimize(header);
  }
}
BENCHMARK(BM_FrameHeaderParse);

static void BM_FrameHeaderSerialize(benchmark::State &state) {
  FrameHeader header(ChannelId::MEDIA_SINK_VIDEO, FrameType::FIRST,
                     EncryptionType::ENCRYPTED, MessageType::CONTROL);

  for (auto _ : state) {
    Data data = header.getData();
    benchmark::DoNotOptimize(data);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_FrameHeaderSerialize);

static void BM_FrameHeaderRoundtrip(benchmark::State &state) {
  for (auto _ : state) {
    FrameHeader original(ChannelId::SENSOR, FrameType::BULK,
                         EncryptionType::PLAIN, MessageType::SPECIFIC);
    Data serialized = original.getData();
    DataConstBuffer buf(serialized);
    FrameHeader parsed(buf);
    benchmark::DoNotOptimize(parsed);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_FrameHeaderRoundtrip);

// -- FrameSize benchmarks --

static void BM_FrameSizeParseShort(benchmark::State &state) {
  // 2-byte short frame size (big-endian 1024)
  uint8_t raw[2] = {0x04, 0x00};
  DataConstBuffer buffer(raw, sizeof(raw));

  for (auto _ : state) {
    FrameSize fs(buffer);
    benchmark::DoNotOptimize(fs);
  }
}
BENCHMARK(BM_FrameSizeParseShort);

static void BM_FrameSizeParseExtended(benchmark::State &state) {
  // 6-byte extended frame size: 2-byte frame size + 4-byte total size
  uint8_t raw[6] = {0x04, 0x00, 0x00, 0x01, 0x00, 0x00};
  DataConstBuffer buffer(raw, sizeof(raw));

  for (auto _ : state) {
    FrameSize fs(buffer);
    benchmark::DoNotOptimize(fs);
  }
}
BENCHMARK(BM_FrameSizeParseExtended);

static void BM_FrameSizeSerializeShort(benchmark::State &state) {
  FrameSize fs(1024);

  for (auto _ : state) {
    Data data = fs.getData();
    benchmark::DoNotOptimize(data);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_FrameSizeSerializeShort);

static void BM_FrameSizeSerializeExtended(benchmark::State &state) {
  FrameSize fs(1024, 65536);

  for (auto _ : state) {
    Data data = fs.getData();
    benchmark::DoNotOptimize(data);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_FrameSizeSerializeExtended);

static void BM_FrameSizeRoundtripExtended(benchmark::State &state) {
  for (auto _ : state) {
    FrameSize original(4096, 131072);
    Data serialized = original.getData();
    DataConstBuffer buf(serialized);
    FrameSize parsed(buf);
    benchmark::DoNotOptimize(parsed.getFrameSize());
    benchmark::DoNotOptimize(parsed.getTotalSize());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_FrameSizeRoundtripExtended);

// -- MessageId benchmarks --

static void BM_MessageIdEncode(benchmark::State &state) {
  MessageId id(0x8001);

  for (auto _ : state) {
    Data data = id.getData();
    benchmark::DoNotOptimize(data);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageIdEncode);

static void BM_MessageIdDecode(benchmark::State &state) {
  // Big-endian representation of 0x8001
  Data raw = {0x80, 0x01};

  for (auto _ : state) {
    MessageId id(raw);
    benchmark::DoNotOptimize(id.getId());
  }
}
BENCHMARK(BM_MessageIdDecode);

static void BM_MessageIdRoundtrip(benchmark::State &state) {
  for (auto _ : state) {
    MessageId original(0x1234);
    Data encoded = original.getData();
    MessageId decoded(encoded);
    benchmark::DoNotOptimize(decoded.getId());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageIdRoundtrip);

// -- Message payload benchmarks --

static void BM_MessageInsertPayloadSmall(benchmark::State &state) {
  Data payload(64, 0xAB);

  for (auto _ : state) {
    Message msg(ChannelId::CONTROL, EncryptionType::PLAIN,
                MessageType::SPECIFIC);
    msg.insertPayload(payload);
    benchmark::DoNotOptimize(msg.getPayload());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageInsertPayloadSmall);

static void BM_MessageInsertPayloadMedium(benchmark::State &state) {
  Data payload(4096, 0xCD);

  for (auto _ : state) {
    Message msg(ChannelId::MEDIA_SINK_VIDEO, EncryptionType::ENCRYPTED,
                MessageType::SPECIFIC);
    msg.insertPayload(payload);
    benchmark::DoNotOptimize(msg.getPayload());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageInsertPayloadMedium);

static void BM_MessageInsertPayloadLarge(benchmark::State &state) {
  Data payload(65536, 0xEF);

  for (auto _ : state) {
    Message msg(ChannelId::MEDIA_SINK_VIDEO, EncryptionType::ENCRYPTED,
                MessageType::SPECIFIC);
    msg.insertPayload(payload);
    benchmark::DoNotOptimize(msg.getPayload());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageInsertPayloadLarge);

static void BM_MessageInsertPayloadBuffer(benchmark::State &state) {
  Data raw(4096, 0xAA);
  DataConstBuffer buffer(raw);

  for (auto _ : state) {
    Message msg(ChannelId::CONTROL, EncryptionType::PLAIN,
                MessageType::SPECIFIC);
    msg.insertPayload(buffer);
    benchmark::DoNotOptimize(msg.getPayload());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageInsertPayloadBuffer);

static void BM_MessageInsertMultiplePayloads(benchmark::State &state) {
  Data chunk(512, 0xBB);

  for (auto _ : state) {
    Message msg(ChannelId::MEDIA_SINK_MEDIA_AUDIO, EncryptionType::PLAIN,
                MessageType::SPECIFIC);
    for (int i = 0; i < 8; ++i) {
      msg.insertPayload(chunk);
    }
    benchmark::DoNotOptimize(msg.getPayload());
    benchmark::ClobberMemory();
  }
}
BENCHMARK(BM_MessageInsertMultiplePayloads);

BENCHMARK_MAIN();
