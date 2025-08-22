# Plugin System with Protocol Buffers

A modular C++-20 plugin system that demonstrates dynamic loading, Protocol Buffers serialization, and unit testing with Google Test, all built and managed by Bazel 8 (Bzlmod).

---

## Contents
1. [What You Get](#what-you-get)  
2. [Why Protocol Buffers?](#why-protocol-buffers)  
3. [Project Layout](#project-layout)  
4. [Quick Start](#quick-start)  
5. [Development Workflow](#development-workflow)  
6. [How It Works](#how-it-works)  
7. [Extending the System](#extending-the-system)  
8. [Troubleshooting](#troubleshooting)  
9. [License](#license)

---

## What You Get
* **Host** application (`host/`) that loads plugins at runtime via `dlopen`.
* **Plugin** sample (`plugin/`) implementing data conversion to a protobuf message.
* **Protocol Buffers** schema (`proto/`) and generated C++ code.
* **Google Test** unit tests for the plugin.
* **Bazel** build files with Bzlmod dependencies.
* **Makefile** for convenient commands.
* **C++20** code using `std::span`, `std::string`, and modern idioms.

---

## Why Protocol Buffers?
| Feature                | Benefit in this Project                          |
|------------------------|--------------------------------------------------|
| Compact binary format  | Faster IPC and smaller payloads                  |
| Strong typing          | Compile-time safety for host ⇆ plugin messages   |
| Schema evolution       | Add new fields without breaking old plugins      |
| Cross-language support | Future-proof if you add Python/Go plugins        |

---

## Project Layout
```

plugin_system/
├── MODULE.bazel          \# Bazel dependencies (protobuf, rules_cc, googletest)
├── BUILD.bazel           \# Global aliases
├── .bazelrc              \# C++20 flag + Bzlmod enable
├── Makefile              \# Helper targets (build, run, test, etc.)
├── README.md             \# ← you are here
│
├── proto/                \# Protobuf definitions
│   ├── demo.proto
│   └── BUILD.bazel
│
├── plugin/               \# Plugin + API + tests
│   ├── api.h
│   ├── fake_plugin.h
│   ├── fake_plugin.cc
│   ├── fake_plugin_test.cc
│   └── BUILD.bazel
│
└── host/                 \# Host application
├── main.cc
└── BUILD.bazel

```

---

## Quick Start
```


# 1. Sync dependencies

bazel sync

# 2. Build host and plugin

bazel build //host:main //plugin:libfake_plugin_so.so

# 3. Run the system (host auto-loads the plugin)

bazel run //host:main

```

Expected output:
```

Loading plugin: bazel-bin/plugin/libfake_plugin_so.so
Plugin API version: 1
Plugin created successfully
Found 1 converter(s)
Topic: /demo/topic
✓ Conversion successful - 12 bytes generated
Publishing to topic '/demo/topic' (12 bytes)

```

Run unit tests:
```

bazel test //plugin:fake_plugin_test

```

---

## Development Workflow
| Action                           | Command                          |
|---------------------------------|----------------------------------|
| Build everything                | `make build`                     |
| Build host only                 | `make host`                      |
| Build plugin only               | `make plugin`                    |
| Run host + plugin               | `make run`                       |
| Execute unit tests              | `make test`                      |
| Clean artifacts                 | `make clean`                     |
| Full dev cycle (clean→build→run)| `make dev`                       |
| Debug build                     | `make debug`                     |

---

## How It Works

### 1. Protocol Buffer Definition (`proto/demo.proto`)
```

syntax = "proto3";
package demo;

message DemoMessage {
int32  counter = 1;
string text    = 2;
}

```
Bazel generates C++ classes (`demo.pb.h/.cc`) used by the converter.

### 2. Plugin API (`plugin/api.h`)
```

struct IConverter {
uint64_t            GetMessageId() const;
std::string         Topic()        const;
bool                Convert(std::span<const uint8_t> raw,
std::vector<uint8_t>\& out);
std::string         GetSchema()    const;
};

```

### 3. Sample Plugin (`plugin/fake_plugin.cc`)
Serializes a `DemoMessage` with hard-coded values and exports C symbols:
```

extern "C" {
IPlugin*   CreatePlugin(IPluginContext* ctx);
void       DestroyPlugin(IPlugin* p);
uint32_t   PluginApiVersion();
}

```

### 4. Host (`host/main.cc`)
1. `dlopen` plugin `.so`
2. Resolve `CreatePlugin`, `DestroyPlugin`, `PluginApiVersion`
3. Call `EnumerateConverters`, invoke `Convert`
4. Publish result via `IPluginContext`

---

## Extending the System

### Add a New Plugin
1. Copy `plugin/fake_plugin.*` → `plugin_new/`.
2. Implement new converter logic.
3. Add `BUILD.bazel` target `libnew_plugin_so.so`.
4. Build:  
   `bazel build //plugin_new:libnew_plugin_so.so`
5. Run:  
   `bazel run //host:main -- bazel-bin/plugin_new/libnew_plugin_so.so`

### Add Fields to Protobuf
1. Append new fields with **new tag numbers** in `demo.proto`.
2. `bazel build //proto:demo_cc_proto` regenerates code.
3. Update plugin & host logic—older binaries remain compatible.

---

## Troubleshooting

| Problem                                   | Fix |
|-------------------------------------------|-----|
| *dlopen failed*                           | Check `.so` path; run `bazel build //plugin:libfake_plugin_so.so` |
| Missing symbols                           | Verify `extern "C"` exports & rebuild plugin |
| C++20 errors                              | Confirm `--cxxopt=-std=c++20` in `.bazelrc` |
| Protobuf version mismatch                 | `bazel clean && bazel sync` |
