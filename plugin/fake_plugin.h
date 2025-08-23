#pragma once
#include "plugin/api.h"
#include <memory>
#include <vector>

class FakeConverter final : public IConverter {
 public:
  uint64_t GetMessageId() const override { return 0xDEADBEEF; }
  std::string Topic() const override { return "/demo/topic"; }
  bool Convert(const std::vector<std::byte> &raw, std::vector<std::byte>& out) override;
  std::string GetSchema() const override;
};

class FakePlugin final : public IPlugin {
 public:
  explicit FakePlugin(IPluginContext* ctx): ctx_(ctx) {}
  void EnumerateConverters(std::vector<IConverter*>& out) override;
 private:
  IPluginContext* ctx_ = nullptr;
  FakeConverter converter_{};
};
