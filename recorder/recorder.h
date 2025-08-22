#pragma once

#include <mcap/writer.hpp>
#include <string>
#include <string_view>
#include <map>  // ← adicionar

class McapRecorder {
public:
    explicit McapRecorder(const std::string& filepath);
    void write(const std::string& topic,
               uint64_t log_time_ns,
               std::string_view payload,
               const std::string& schema_name,
               const std::string& schema_text);
    void close();
private:
    mcap::McapWriter writer_;
};