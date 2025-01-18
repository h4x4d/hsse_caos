#pragma once
#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>

namespace constants {
constexpr size_t kMmapThreshold = 131'072;
constexpr size_t kMinSize = 32;
constexpr size_t kFastMax = 104;
constexpr size_t kFastConsolidate = 65'536;
constexpr size_t kMaxSize = 33'554'432;

constexpr size_t kFastBinsSize = 10;
constexpr size_t kBinsSize = 126;

constexpr double kBigBinBase = 1.125;
constexpr size_t kMaxSmallBinSize = 1024;
}  // namespace constants


namespace node_ptr {
using BytePtr = std::byte*;

void* Advance(void* ptr, int number);

size_t Difference(void* first, void* second);

size_t GetMeta(void* ptr);

size_t GetSize(void* ptr);

void SetMeta(void* ptr, size_t new_meta);

void*& Next(void* ptr);

void*& Prev(void* ptr);

void* End(void* ptr);

bool IsNumberValid(size_t number);

bool IsValid(void* ptr);

bool IsFree(void* ptr);

bool IsMmaped(void* ptr);

void SetOccupied(void* ptr, bool free);
}  // namespace node_ptr

namespace utils {
double Log(double number, double base);

size_t GetIndex(size_t size);

void* GetMmap(size_t size);

size_t GetChunkSize(size_t size);

void* GetMremap(void* ptr, size_t size);

void* GetFrom(size_t size, void** container);

void* GetFast(size_t size);

void* GetBin(size_t size);

void* GetFromHeap(size_t size);

void AddTo(void* ptr, void** container);

void AddToFast(void* ptr);

void AddToBin(void* ptr);

void FreeMmap(void* ptr);

void* MergeNeighbours(void* ptr);

void FreePtr(void* ptr);

void ClearFast();
}  // namespace utils

namespace stdlike {
void* malloc(size_t size);

void* calloc(size_t size, size_t amount);

void* realloc(void* ptr, size_t new_size);

void free(void* ptr);
}  // namespace stdlike