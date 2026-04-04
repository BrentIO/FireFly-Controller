#pragma once

#include <ArduinoJson.h>

/**
 * Custom ArduinoJSON allocator that uses PSRAM (ps_malloc/ps_realloc) when
 * available, falling back to regular heap automatically when PSRAM is absent.
 * Use SpiRamJsonDocument in place of JsonDocument for large allocations.
 */
struct SpiRamAllocator {
    void* allocate(size_t size) { return ps_malloc(size); }
    void deallocate(void* p) { free(p); }
    void* reallocate(void* p, size_t n) { return ps_realloc(p, n); }
};

using SpiRamJsonDocument = BasicJsonDocument<SpiRamAllocator>;
