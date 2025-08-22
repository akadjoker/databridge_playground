<img src="https://r2cdn.perplexity.ai/pplx-full-logo-primary-dark%402x.png" style="height:64px;margin-right:32px"/>

# Plugin System with Protocol Buffers \& MCAP Recording

A modular C++-20 plugin system that demonstrates dynamic loading, Protocol Buffers serialization, MCAP data recording, and unit testing with Google Test, all built and managed by Bazel 8 (Bzlmod).

***

## Contents

1. [What You Get](#what-you-get)
2. [Why Protocol Buffers \& MCAP?](#why-protocol-buffers--mcap)
3. [Project Layout](#project-layout)
4. [Quick Start](#quick-start)
5. [Development Workflow](#development-workflow)
6. [How It Works](#how-it-works)
7. [MCAP Recording \& Visualization](#mcap-recording--visualization)
8. [Extending the System](#extending-the-system)
9. [Troubleshooting](#troubleshooting)
10. [License](#license)

***

## What You Get

* **Host** application (`host/`) that loads plugins at runtime via `dlopen`.
* **Plugin** sample (`plugin/`) implementing data conversion to a protobuf message.
* **MCAP Recorder** (`record_tool/`) that captures plugin data for visualization.
* **Protocol Buffers** schema (`proto/`) and generated C++ code.
* **MCAP Integration** with Foxglove Studio for data analysis.
* **Google Test** unit tests for the plugin.
* **Bazel** build files with Bzlmod dependencies.
* **Makefile** for convenient commands.
* **C++20** code using `std::span`, `std::string`, and modern idioms.

***

## Why Protocol Buffers \& MCAP?

| Feature | Benefit in this Project |
| :-- | :-- |
| **Protocol Buffers** |  |
| Compact binary format | Faster IPC and smaller payloads |
| Strong typing | Compile-time safety for host ⇆ plugin messages |
| Schema evolution | Add new fields without breaking old plugins |
| Cross-language support | Future-proof if you add Python/Go plugins |
| **MCAP Recording** |  |
| Industry standard | Compatible with ROS, robotics ecosystem |
| Foxglove integration | Professional data visualization and analysis |
| Self-describing | Schema and metadata embedded in files |
| Efficient storage | Optimized for time-series data |


***

## Project Layout

```
plugin_system/
├── MODULE.bazel          # Bazel dependencies (protobuf, rules_cc, googletest)
├── BUILD.bazel           # Global aliases
├── .bazelrc              # C++20 flag + Bzlmod enable
├── Makefile              # Helper targets (build, run, test, record, etc.)
├── README.md             # ← you are here
│
├── proto/                # Protobuf definitions
│   ├── demo.proto
│   └── BUILD.bazel
│
├── plugin/               # Plugin + API + tests
│   ├── api.h
│   ├── fake_plugin.h
│   ├── fake_plugin.cc
│   ├── fake_plugin_test.cc
│   └── BUILD.bazel
│
├── host/                 # Host application
│   ├── main.cc
│   └── BUILD.bazel
│
├── recorder/             # MCAP recording library
│   ├── recorder.h
│   ├── recorder.cc
│   └── BUILD.bazel
│
├── record_tool/          # MCAP recording application
│   ├── record_mcap.cc
│   └── BUILD.bazel
│
└── third_party/          # External dependencies
    └── mcap/             # MCAP headers (header-only)
```


***

## Quick Start

```bash
# 1. Install system dependencies
sudo apt update
sudo apt install -y build-essential libzstd-dev liblz4-dev

# 2. Sync Bazel dependencies
bazel sync

# 3. Build everything
bazel build //host:main //plugin:libfake_plugin_so.so //record_tool:record_mcap

# 4. Test the plugin system
bazel run //host:main

# 5. Record plugin data to MCAP file
bazel run //record_tool:record_mcap -- \
  $(bazel info bazel-bin)/plugin/libfake_plugin_so.so recording.mcap
```

Expected output:

```
=== Plugin System Host ===
Loading plugin: bazel-bin/plugin/libfake_plugin_so.so
Plugin API version: 1
Plugin created successfully
Found 1 converter(s)
Topic: /demo/topic
✓ Conversion successful - 12 bytes generated
Publishing to topic '/demo/topic' (12 bytes)
✅ Written recording.mcap
```

Run unit tests:

```bash
bazel test //plugin:fake_plugin_test
```


***

## Development Workflow

| Action | Command |
| :-- | :-- |
| Build everything | `make build` |
| Build host only | `make host` |
| Build plugin only | `make plugin` |
| Run host + plugin | `make run` |
| **Record to MCAP** | **`make record`** |
| Execute unit tests | `make test` |
| Clean artifacts | `make clean` |
| Full dev cycle (clean→build→run) | `make dev` |
| Debug build | `make debug` |

### MCAP Analysis Commands

```bash
# Install MCAP CLI (optional)
pip install mcap-cli

# Inspect recorded data
mcap info recording.mcap
mcap list recording.mcap
mcap validate recording.mcap

# Open in Foxglove Studio
# Download from: https://foxglove.dev/studio
# File → Open local file → recording.mcap
```


***

## How It Works

### 1. Protocol Buffer Definition (`proto/demo.proto`)

```protobuf
syntax = "proto3";
package demo;

message DemoMessage {
  int32  counter = 1;
  string text    = 2;
}
```

Bazel generates C++ classes (`demo.pb.h/.cc`) used by the converter.

### 2. Plugin API (`plugin/api.h`)

```cpp
struct IPluginContext {
    virtual void LogInfo(const char* msg) = 0;
    virtual void Publish(const std::string& topic, 
                        const void* data, size_t size) = 0;
};

struct IConverter {
    virtual uint64_t     GetMessageId() const = 0;
    virtual std::string  Topic()        const = 0;
    virtual bool         Convert(std::span<const uint8_t> raw,
                                std::vector<uint8_t>& out) = 0;
    virtual std::string  GetSchema()    const = 0;
};
```


### 3. Sample Plugin (`plugin/fake_plugin.cc`)

Serializes a `DemoMessage` with hard-coded values and exports C symbols:

```cpp
extern "C" {
    IPlugin*  CreatePlugin(IPluginContext* ctx);
    void      DestroyPlugin(IPlugin* p);
    uint32_t  PluginApiVersion();
}
```


### 4. Host (`host/main.cc`)

1. `dlopen` plugin `.so`
2. Resolve `CreatePlugin`, `DestroyPlugin`, `PluginApiVersion`
3. Call `EnumerateConverters`, invoke `Convert`
4. Publish result via `IPluginContext`

***

## MCAP Recording \& Visualization

### MCAP Recorder Architecture

```
Plugin → Host → IPluginContext::Publish() → McapRecorder → MCAP File
                                              ↓
                                         Foxglove Studio
```


### Recording Process

1. **Data Flow**: Plugin generates protobuf → Host captures via `Publish()` → MCAP writer stores with schema
2. **Schema Management**: Protobuf schemas automatically embedded in MCAP metadata
3. **Topic Routing**: Each plugin topic becomes an MCAP channel
4. **Timestamping**: High-resolution timestamps for precise data analysis

### Visualization in Foxglove Studio

#### Setup

1. Download [Foxglove Studio](https://foxglove.dev/studio)
2. Open recorded MCAP file: **File → Open local file**
3. Configure visualization panels

#### Recommended Panels

| Panel Type | Purpose | Configuration |
| :-- | :-- | :-- |
| Raw Messages | View JSON/Protobuf data | Topic: `/demo/topic` |
| Plot | Time-series visualization | Field: `counter` |
| Table | Tabular data display | All fields |
| 3D | Spatial data (if applicable) | Custom transforms |

#### Example Data Structure

```json
{
  "counter": 42,
  "text": "Hello World",
  "timestamp": 1642678800123456789,
  "raw_data": [8, 42, 18, 11, 72, 101, 108, 108, 111, 32, 87, 111, 114, 108, 100]
}
```


### MCAP File Analysis

```bash
# File metadata
mcap info recording.mcap
# Output:
# library: libmcap 2.0.2
# messages: 1
# duration: 0.001s
# channels: (1) /demo/topic  1 msgs : demo.DemoMessage

# Message content
mcap list recording.mcap --format=json
# Shows actual message data with timestamps

# File validation
mcap validate recording.mcap
# Checks for corruption or format errors
```


***

## Extending the System

### Add a New Plugin

1. Copy `plugin/fake_plugin.*` → `plugin_new/`.
2. Implement new converter logic.
3. Add `BUILD.bazel` target `libnew_plugin_so.so`.
4. Build:

```bash
bazel build //plugin_new:libnew_plugin_so.so
```

5. Test with host:

```bash
bazel run //host:main -- bazel-bin/plugin_new/libnew_plugin_so.so
```

6. Record data:

```bash
bazel run //record_tool:record_mcap -- \
  bazel-bin/plugin_new/libnew_plugin_so.so new_recording.mcap
```


### Add Fields to Protobuf

1. Append new fields with **new tag numbers** in `demo.proto`.
2. `bazel build //proto:demo_cc_proto` regenerates code.
3. Update plugin \& host logic—older binaries remain compatible.
4. MCAP files automatically include updated schemas.

### Custom MCAP Schemas

```cpp
// In your plugin converter
std::string GetSchema() const override {
    return R"(
        syntax = "proto3";
        package mypackage;
        message CustomMessage {
            double temperature = 1;
            string sensor_id = 2;
            repeated float readings = 3;
        }
    )";
}
```


### Multiple Topic Recording

```cpp
// Plugin can publish to multiple topics
ctx->Publish("/sensor/temperature", temp_data, temp_size);
ctx->Publish("/sensor/pressure", press_data, press_size);
ctx->Publish("/system/status", status_data, status_size);
```


***

## Troubleshooting

| Problem | Fix |
| :-- | :-- |
| **General Plugin Issues** |  |
| *dlopen failed* | Check `.so` path; run `bazel build //plugin:libfake_plugin_so.so` |
| Missing symbols | Verify `extern "C"` exports \& rebuild plugin |
| C++20 errors | Confirm `--cxxopt=-std=c++20` in `.bazelrc` |
| Protobuf version mismatch | `bazel clean && bazel sync` |
| **MCAP Recording Issues** |  |
| *Recording file empty in Foxglove* | Check `mcap info recording.mcap` for message count |
| *"invalid type 7 at offset 13"* | Plugin generating invalid protobuf data; use JSON format |
| *"no such type DemoMessage"* | Schema mismatch; use empty schema for JSON: `rec.write(topic, data, "", "")` |
| *"index out of range"* | Buffer overflow in JSON generation; limit data size |
| **Dependencies** |  |
| *libzstd-dev not found* | `sudo apt install libzstd-dev liblz4-dev` |
| *MCAP compilation errors* | Ensure headers in `third_party/mcap/include/` |
| **Foxglove Studio** |  |
| *No data visible* | Check timeline, add Raw Messages panel for `/demo/topic` |
| *Schema errors* | Use JSON format instead of protobuf for debugging |

### Debug Mode

```bash
# Verbose MCAP recording
bazel run -c dbg //record_tool:record_mcap -- \
  $(bazel info bazel-bin)/plugin/libfake_plugin_so.so debug.mcap 2>&1 | tee debug.log

# Check what's actually in the MCAP file
mcap list debug.mcap --format=json | head -50
```


### Validation Workflow

```bash
# 1. Test plugin independently
bazel run //host:main

# 2. Record to MCAP
bazel run //record_tool:record_mcap -- plugin.so test.mcap

# 3. Validate MCAP structure
mcap validate test.mcap

# 4. Check content
mcap info test.mcap
mcap list test.mcap

# 5. Open in Foxglove Studio
```


***

## Dependencies

### Build System

- **Bazel 8.0+** with Bzlmod enabled
- **GCC 11+** or **Clang 14+** (C++20 support)


### External Libraries (via Bazel)

- **Protocol Buffers 27.1**: Message serialization
- **Google Test 1.14.0**: Unit testing framework
- **rules_cc 0.0.9**: C++ build rules


### System Libraries

- **libzstd-dev**: ZSTD compression support
- **liblz4-dev**: LZ4 compression support


### Optional Tools

- **mcap-cli**: MCAP file analysis (`pip install mcap-cli`)
- **Foxglove Studio**: Data visualization ([Download](https://foxglove.dev/studio))

***

## License

[Specify your license here]

***

## Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature/mcap-enhancement`
3. Implement changes with tests
4. Update documentation
5. Submit pull request

### Development Guidelines

- Follow C++20 best practices
- Add unit tests for new plugins
- Document MCAP schema changes
- Test with Foxglove Studio integration

***

This plugin system provides a comprehensive foundation for modular data processing with modern C++ practices, Protocol Buffers serialization, and professional-grade data recording and visualization capabilities.
