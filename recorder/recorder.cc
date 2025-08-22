 
 

#include "recorder/recorder.h"
#include <chrono>
#include <stdexcept>
#include <iostream>
 
McapRecorder::McapRecorder(const std::string& path) {
    auto options = mcap::McapWriterOptions("");   
    options.compression = mcap::Compression::None;
    auto status = writer_.open(path, options);
    if (!status.ok()) {
        throw std::runtime_error("Failed to open MCAP file: " + status.message);
    }
}

void McapRecorder::write(const std::string& topic,
                         uint64_t log_time_ns,
                         std::string_view payload,
                         const std::string& schema_name,
                         const std::string& schema_text) {
    
    // Criar schema (só uma vez por schema_name)
    static std::map<std::string, mcap::SchemaId> schemas;
    static std::map<std::string, mcap::ChannelId> channels;
    
    mcap::SchemaId schema_id;
    if (schemas.find(schema_name) == schemas.end()) {
        mcap::Schema schema(schema_name, "protobuf", schema_text);
        writer_.addSchema(schema);
        schemas[schema_name] = schema.id;
        schema_id = schema.id;
    } else {
        schema_id = schemas[schema_name];
    }
    
    // Criar canal (só uma vez por topic)
    mcap::ChannelId channel_id;
    if (channels.find(topic) == channels.end()) {
        mcap::Channel channel(topic, "protobuf", schema_id);
        writer_.addChannel(channel);
        channels[topic] = channel.id;
        channel_id = channel.id;
    } else {
        channel_id = channels[topic];
    }
    
    // Criar mensagem
    mcap::Message msg;
    msg.channelId = channel_id;
    msg.sequence = 0;
    msg.logTime = log_time_ns;
    msg.publishTime = log_time_ns;
    msg.data = reinterpret_cast<const std::byte*>(payload.data());
    msg.dataSize = payload.size();
    
    auto res = writer_.write(msg);  // ← verificar resultado
    if (!res.ok()) {
        std::cerr << "Failed to write message: " << res.message << std::endl;
    }
}

void McapRecorder::close() {
    writer_.close();
}
