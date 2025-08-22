#include "plugin/fake_plugin.h"
#include "proto/demo.pb.h"
#include <cstring>

using namespace std;

bool FakeConverter::Convert(span<const std::byte> /*raw*/, vector<std::byte>& out) {
  demo::DemoMessage msg;
  msg.set_counter(42);
  msg.set_text("hello");

  string s;
  msg.SerializeToString(&s);
  out.resize(s.size());
  memcpy(out.data(), s.data(), s.size());
  return true;
}

std::string FakeConverter::GetSchema() const {
  return R"(syntax = "proto3"; package demo; message DemoMessage { int32 counter = 1; string text = 2; })";
}

void FakePlugin::EnumerateConverters(vector<IConverter*>& out) {
  out.push_back(&converter_);
}

namespace {
  struct DummyCtx : IPluginContext {
    void LogInfo(const char* /*msg*/) override {}
    void Publish(const std::string&, const void*, size_t) override {}
  };
}

extern "C" IPlugin* CreatePlugin(IPluginContext* ctx) {
  if (!ctx) {
    static DummyCtx dummy;
    ctx = &dummy;
  }
  return new FakePlugin(ctx);
}

extern "C" void DestroyPlugin(IPlugin* p) { delete p; }
extern "C" uint32_t PluginApiVersion() { return 1; }
