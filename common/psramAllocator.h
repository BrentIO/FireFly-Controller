#pragma once

#include <ArduinoJson.h>

/**
 * Custom ArduinoJSON v7 allocator that uses PSRAM (ps_malloc/ps_realloc)
 * when available, falling back to regular heap automatically when PSRAM
 * is absent. Pass a pointer to the global spiRamAllocator instance to
 * JsonDocument to use PSRAM-backed allocation:
 *
 *   JsonDocument doc(&spiRamAllocator);
 */
class SpiRamAllocator : public ArduinoJson::Allocator {
public:
    void* allocate(size_t size) override { return ps_malloc(size); }
    void deallocate(void* ptr) override { free(ptr); }
    void* reallocate(void* ptr, size_t new_size) override { return ps_realloc(ptr, new_size); }
};

inline SpiRamAllocator spiRamAllocator;
