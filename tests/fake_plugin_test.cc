#include <gtest/gtest.h>
#include "plugin/fake_plugin.h"
#include "proto/demo.pb.h"

TEST(FakePlugin, ConvertProducesValidProto) {
  FakeConverter c;
  std::vector<std::byte> out;
  std::span<const std::byte> in{};

  ASSERT_TRUE(c.Convert(in, out));

  demo::DemoMessage msg;
  ASSERT_TRUE(msg.ParseFromArray(out.data(), static_cast<int>(out.size())));
  EXPECT_EQ(msg.counter(), 42);
  EXPECT_EQ(msg.text(), "hello");
}
