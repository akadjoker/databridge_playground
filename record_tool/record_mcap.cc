#define MCAP_IMPLEMENTATION
#include <mcap/writer.hpp>

#include "plugin/api.h"
#include "recorder/recorder.h"
#include <dlfcn.h>
#include <chrono>
#include <iostream>
#include <vector>
 #include <sstream>

static uint64_t now_ns() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(system_clock::now().time_since_epoch()).count();
}

struct HostCtx : IPluginContext {
    explicit HostCtx(McapRecorder& r) : recorder(r) {}
    void LogInfo(const char* msg) override { std::cerr << "[plugin] " << msg << '\n'; }
    

    void Publish(const std::string& topic, const void* data, size_t size) override 
    {
    // JSON estruturado com schema Foxglove Log
    std::ostringstream json;
    json << "{"
         << "\"timestamp\": {\"sec\": " << (now_ns() / 1000000000) 
         << ", \"nsec\": " << (now_ns() % 1000000000) << "},"
         << "\"level\": 1,"
         << "\"message\": \"Data received: " << size << " bytes\","
         << "\"name\": \"plugin_system\""
         << "}";
    
    std::string json_str = json.str();
    recorder.write(topic, now_ns(), json_str, 
              "foxglove.Log",  // ← Schema Foxglove oficial
              R"({
                "type": "object",
                "properties": {
                  "timestamp": {"type": "object"},
                  "level": {"type": "integer"}, 
                  "message": {"type": "string"},
                  "name": {"type": "string"}
                }
              })");
}


    McapRecorder& recorder;
};

int main(int argc, char** argv) {
    const char* so_path   = (argc > 1) ? argv[1] : "bazel-bin/plugin/libfake_plugin_so.so";
    const char* bag_path  = (argc > 2) ? argv[2] : "recording.mcap";

    

    try {
        McapRecorder recorder(bag_path);
        HostCtx ctx(recorder);

        void* h = dlopen(so_path, RTLD_NOW);
        if (!h) { std::cerr << "dlopen failed: " << dlerror() << '\n'; return 1; }


      std::cout << "=== MCAP Recorder ===" << std::endl;
      std::cout << "Plugin: " << so_path << std::endl;
      std::cout << "Output: " << bag_path << std::endl;  

        auto create  = (IPlugin*(*)(IPluginContext*)) dlsym(h, "CreatePlugin");
        auto destroy = (void(*)(IPlugin*))            dlsym(h, "DestroyPlugin");
        if (!create || !destroy) { std::cerr << "dlsym failed\n"; return 2; }

        IPlugin* p = create(&ctx);
        std::vector<IConverter*> convs;
        p->EnumerateConverters(convs);
        if (convs.empty()) { std::cerr << "No converters\n"; return 3; }

      
        std::vector<std::byte> out;
        std::vector<std::byte> din;
        convs.front()->Convert(din, out);                         // vazio  mensagem demo
        ctx.Publish(convs.front()->Topic(), out.data(), out.size());

        destroy(p);
        dlclose(h);
        recorder.close();
        std::cout << "Written " << bag_path << '\n';
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 4;
    }
    return 0;
}
