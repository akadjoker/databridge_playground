#include "plugin/api.h"
#include <iostream>
#include <dlfcn.h>
#include <vector>
#include <cstring>


struct HostCtx : IPluginContext {
    void LogInfo(const char* msg) override { 
        std::cout << "[PLUGIN] " << msg << std::endl; 
    }
    
    void Publish(const std::string& topic, const void* data, size_t size) override {
        std::cout << "Publishing to topic '" << topic << "' (" << size << " bytes)" << std::endl;
        
        // Hexdump dos primeiros bytes para debug
        const uint8_t* bytes = static_cast<const uint8_t*>(data);
        std::cout << "Data (hex): ";
        for (size_t i = 0; i < std::min(size, size_t(16)); ++i) {
            printf("%02x ", bytes[i]);
        }
        if (size > 16) std::cout << "...";
        std::cout << std::endl;
    }
};

int main(int argc, char** argv) {
    const char* so_path = (argc > 1) ? argv[1] : "bazel-bin/plugin/libfake_plugin_so.so";
    
    std::cout << "=== Plugin System Host ===" << std::endl;
    std::cout << "Loading plugin: " << so_path << std::endl;
    
    // Carregar plugin
    void* handle = dlopen(so_path, RTLD_NOW);
    if (!handle) {
        std::cerr << "ERROR: dlopen failed: " << dlerror() << std::endl;
        return 1;
    }
    
    // Resolver símbolos
    auto create_fn = (IPlugin*(*)(IPluginContext*)) dlsym(handle, "CreatePlugin");
    auto destroy_fn = (void(*)(IPlugin*)) dlsym(handle, "DestroyPlugin"); 
    auto version_fn = (uint32_t(*)()) dlsym(handle, "PluginApiVersion");
    
    if (!create_fn || !destroy_fn || !version_fn) {
        std::cerr << "ERROR: dlsym failed - missing symbols" << std::endl;
        dlclose(handle);
        return 2;
    }
    
    // Verificar versão da API
    uint32_t api_version = version_fn();
    std::cout << "Plugin API version: " << api_version << std::endl;
    
    if (api_version != 1) {
        std::cerr << "ERROR: Unsupported API version " << api_version << std::endl;
        dlclose(handle);
        return 3;
    }
    
    // Criar plugin
    HostCtx ctx;
    IPlugin* plugin = create_fn(&ctx);
    if (!plugin) {
        std::cerr << "ERROR: CreatePlugin returned null" << std::endl;
        dlclose(handle);
        return 4;
    }
    
    std::cout << "Plugin created successfully" << std::endl;
    
    // Enumerar converters
    std::vector<IConverter*> converters;
    plugin->EnumerateConverters(converters);
    
    std::cout << "Found " << converters.size() << " converter(s)" << std::endl;
    
    if (converters.empty()) {
        std::cerr << "ERROR: No converters available" << std::endl;
        destroy_fn(plugin);
        dlclose(handle);
        return 5;
    }
    
 
    for (size_t i = 0; i < converters.size(); ++i) {
        IConverter* conv = converters[i];
        
        std::cout << "\n--- Converter " << (i+1) << " ---" << std::endl;
        std::cout << "Topic: " << conv->Topic() << std::endl;
        std::printf("Message ID: 0x%016lx\n", conv->GetMessageId());
        std::cout << "Schema: " << conv->GetSchema() << std::endl;
        
        // Testar conversão
        std::vector<std::byte> output;
        std::vector<std::byte> input{}; // empty input
        
        std::cout << "Converting..." << std::endl;
        bool success = conv->Convert(input, output);
        
        if (success) {
            std::cout << " Conversion successful - " << output.size() << " bytes generated" << std::endl;
            
            // Publicar via context
            ctx.Publish(conv->Topic(), output.data(), output.size());
        } else {
            std::cout << " Conversion failed" << std::endl;
        }
    }
    
  
    std::cout << "\nCleaning up..." << std::endl;
    destroy_fn(plugin);
    dlclose(handle);
    
    std::cout << "Done!" << std::endl;
    return 0;
}

