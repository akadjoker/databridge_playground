.PHONY: help build run test clean dev

.DEFAULT_GOAL := help

BAZEL := bazel
PLUGIN_SO := bazel-bin/plugin/libfake_plugin_so.so
HOST_BIN := bazel-bin/host/main

help: ## Mostra comandos disponíveis
	@echo "Comandos disponíveis:"
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  %-12s %s\n", $$1, $$2}'


record: ## Compila e gera recording.mcap
	$(BAZEL) build //plugin:libfake_plugin_so.so //record_tool:record_mcap
	$(BAZEL) run //record_tool:record_mcap -- $$(bazel info bazel-bin)/plugin/libfake_plugin_so.so recording.mcap


view: ## Mostra estatísticas básicas (precisa de mcap-cli instalado)
	mcap info recording.mcap || echo "Instale a CLI: pip install mcap"		

build: ## Compila host e plugin
	$(BAZEL) build //host:main //plugin:libfake_plugin_so.so

host: ## Compila apenas host
	$(BAZEL) build //host:main

plugin: ## Compila apenas plugin
	$(BAZEL) build //plugin:libfake_plugin_so.so

proto: ## Compila apenas proto
	$(BAZEL) build //proto:demo_cc_proto

run: build ## Executa host carregando plugin
	@echo "Executando host com plugin..."
	$(HOST_BIN) $(PLUGIN_SO)

test: ## Executa testes do plugin
	$(BAZEL) test //plugin:fake_plugin_test

test-verbose: ## Testes com output detalhado
	$(BAZEL) test //plugin:fake_plugin_test --test_output=all

test-all: ## Executa todos os testes
	$(BAZEL) test //...

clean: ## Limpa build
	$(BAZEL) clean

clean-all: ## Limpa tudo
	$(BAZEL) clean --expunge

deps: ## Sincroniza dependências
	$(BAZEL) sync

dev: clean build test run ## Ciclo completo

debug: ## Build debug
	$(BAZEL) build //host:main //plugin:libfake_plugin_so.so --compilation_mode=dbg

release: ## Build otimizado  
	$(BAZEL) build //host:main //plugin:libfake_plugin_so.so --compilation_mode=opt

info: ## Info do projeto
	@echo "Host: $(HOST_BIN)"
	@echo "Plugin: $(PLUGIN_SO)"
	$(BAZEL) info

query: ## Lista todos os targets
	$(BAZEL) query //...

# Comandos modulares
build-host: ## Apenas host
	$(BAZEL) build //host:main

build-plugin: ## Apenas plugin
	$(BAZEL) build //plugin:libfake_plugin_so.so

build-proto: ## Apenas proto
	$(BAZEL) build //proto:demo_cc_proto

# Atalhos
b: build
r: run
t: test
c: clean
h: host
p: plugin

