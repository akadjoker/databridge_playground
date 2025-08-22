#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

struct IPluginContext {
  virtual void LogInfo(const char* msg) = 0;
  virtual void Publish(const std::string& topic, const void* data, size_t size) = 0;
  virtual ~IPluginContext() = default;
};

struct IConverter {
  virtual ~IConverter() = default;
  virtual uint64_t GetMessageId() const = 0;
  virtual std::string Topic() const = 0;
  virtual bool Convert(std::span<const std::byte> raw, std::vector<std::byte>& out) = 0;
  virtual std::string GetSchema() const = 0;
};

struct IPlugin {
  virtual ~IPlugin() = default;
  virtual void EnumerateConverters(std::vector<IConverter*>& out) = 0;
  virtual void Configure(const std::string& yaml) {}
};

extern "C" {
  IPlugin* CreatePlugin(IPluginContext* ctx);
  void DestroyPlugin(IPlugin* p);
  uint32_t PluginApiVersion();
}
