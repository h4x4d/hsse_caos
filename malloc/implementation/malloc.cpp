#include <cmath>
#include <cstring>
#include <iostream>

#include <cstddef>
#include <sys/mman.h>
#include <unistd.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "malloc.hpp"

namespace node_ptr {
using BytePtr = std::byte*;

void* Advance(void* ptr, int number) {
    return static_cast<BytePtr>(ptr) + number;
}

size_t Difference(void* first, void* second) {
    return static_cast<BytePtr>(first) - static_cast<BytePtr>(second);
}

size_t GetMeta(void* ptr) {
    return *static_cast<size_t*>(Advance(ptr, -8));
}

size_t GetSize(void* ptr) {
    return GetMeta(ptr) & (2 * constants::kMaxSize - 4);
}

void*& Next(void* ptr) {
    return *static_cast<void**>(ptr);
}

void*& Prev(void* ptr) {
    return *static_cast<void**>(Advance(ptr, 8));
}

void* End(void* ptr) {
    return Advance(ptr, GetSize(ptr) - 8);
}

bool IsNumberValid(size_t number) {
    return (number > 0) && (number <= constants::kMaxSize + 3);
}

bool IsValid(void* ptr) {
    return IsNumberValid(GetMeta(ptr));
}

bool IsFree(void* ptr) {
    return GetMeta(ptr) % 2 == 0;
}

bool IsMmaped(void* ptr) {
    return (GetMeta(ptr) / 2) % 2 == 1;
}

void SetMeta(void* ptr, size_t new_meta) {
    *static_cast<size_t*>(Advance(ptr, -8)) = new_meta;
    *static_cast<size_t*>(Advance(ptr, GetSize(ptr) - 16)) = new_meta;
}

void SetOccupied(void* ptr, bool occupied) {
    if (node_ptr::IsFree(ptr) == occupied) {
        node_ptr::SetMeta(ptr, node_ptr::GetMeta(ptr) ^ 1);
    }
}

}  // namespace node_ptr

namespace utils {
static void* fast_bins[constants::kFastBinsSize] = {};
static void* bins[constants::kBinsSize] = {};

void* heap_begin = nullptr;
void* heap_first = nullptr;
void* heap_last = nullptr;

double Log(double number, double base) {
    return std::log(number) / std::log(base);
}

size_t GetIndex(size_t size) {
    if (size <= constants::kMaxSmallBinSize) {
        int index = size / 16;
        return static_cast<size_t>(std::max(0, index - 2));
    }
    return 62 + static_cast<size_t>(Log(static_cast<double>(size) / constants::kMaxSmallBinSize,
                                        constants::kBigBinBase));
}

void* GetMmap(size_t size) {
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr != nullptr) {
        ptr = node_ptr::Advance(ptr, 8);
        node_ptr::SetMeta(ptr, size + 3);
    }
    return ptr;
}

size_t GetChunkSize(size_t size) {
    if (size < 32) {
        return 32;
    }
    return (size + 15) / 16 * 16;
}

void* GetMremap(void* ptr, size_t size) {
    ptr = mremap(node_ptr::Advance(ptr, -8), node_ptr::GetSize(ptr), size, PROT_READ | PROT_WRITE);
    node_ptr::SetMeta(ptr, size + 3);
    return node_ptr::Advance(ptr, 8);
    //  return GetMmap(size);
}

void* GetFrom(size_t size, void** container, size_t container_size) {
    size_t index = GetIndex(size);
    void* ptr = nullptr;
    while (ptr == nullptr && index < container_size) {
        if (container[index] != nullptr) {
            ptr = container[index];
            container[index] = node_ptr::Next(ptr);
            if (container[index] != nullptr) {
                node_ptr::Prev(container[index]) = nullptr;
            }
        }
        ++index;
    }
    return ptr;
}

void* GetFromHeap(size_t size) {
    if (heap_first == nullptr) {
        heap_first = node_ptr::Advance(sbrk(0), 8);
        heap_begin = heap_first;
        heap_last = sbrk(constants::kMmapThreshold);
    }
    if (node_ptr::Difference(heap_first, heap_last) < size) {
        heap_last = sbrk(constants::kMmapThreshold);
    }
    void* ptr = node_ptr::Advance(heap_first, 8);
    node_ptr::SetMeta(ptr, size + 1);
    heap_first = node_ptr::Advance(heap_first, size);
    return ptr;
}

void AddTo(void* ptr, void** container) {
    size_t size = node_ptr::GetSize(ptr);
    size_t index = GetIndex(size);

    void* last = container[index];
    if (container[index] != nullptr) {
        node_ptr::Prev(container[index]) = ptr;
    }
    container[index] = ptr;
    node_ptr::Next(ptr) = last;
    node_ptr::Prev(ptr) = nullptr;
}

void AddToBin(void* ptr) {
    node_ptr::SetOccupied(ptr, 0);
    AddTo(ptr, bins);
}

void DeleteFromBucket(void* ptr) {
    if (node_ptr::Prev(ptr) != nullptr) {
        node_ptr::Next(node_ptr::Prev(ptr)) = node_ptr::Next(ptr);
    } else {
        bins[GetIndex(node_ptr::GetSize(ptr))] = node_ptr::Next(ptr);
    }
    if (node_ptr::Next(ptr) != nullptr) {
        node_ptr::Prev(node_ptr::Next(ptr)) = node_ptr::Prev(ptr);
    }
}

void* MergeNeighbours(void* ptr) {
    if (node_ptr::Advance(ptr, -8) != heap_begin) {
        size_t prev_meta = *static_cast<size_t*>(node_ptr::Advance(ptr, -16));
        if (node_ptr::IsNumberValid(prev_meta)) {
            size_t prev_size = prev_meta & (2 * constants::kMaxSize - 4);
            void* prev = node_ptr::Advance(ptr, -prev_size);
            if (node_ptr::IsFree(prev)) {
                DeleteFromBucket(prev);
                node_ptr::SetMeta(prev, node_ptr::GetSize(prev) + node_ptr::GetSize(ptr));
                ptr = prev;
            }
        }
    }
    if (node_ptr::End(ptr) != heap_first) {
        size_t next_meta = *static_cast<size_t*>(node_ptr::End(ptr));
        if (node_ptr::IsNumberValid(next_meta)) {
            void* next = node_ptr::Advance(node_ptr::End(ptr), 8);
            if (node_ptr::IsFree(next)) {
                DeleteFromBucket(next);
                node_ptr::SetMeta(ptr, node_ptr::GetSize(next) + node_ptr::GetSize(ptr));
            }
        }
    }
    return ptr;
}

void FreePtr(void* ptr) {
    ptr = utils::MergeNeighbours(ptr);
    if (node_ptr::End(ptr) == utils::heap_first) {
        utils::heap_first = node_ptr::Advance(ptr, -8);
    } else {
        utils::AddToBin(ptr);
    }
}

void ClearFast() {
    for (auto*& ptr : fast_bins) {
        void* now = ptr;
        while (now != nullptr) {
            void* next = node_ptr::Next(now);
            FreePtr(now);
            now = next;
        }
        ptr = nullptr;
    }
}

void* GetFast(size_t size) {
    return GetFrom(size, fast_bins, 10);
}

void* GetBin(size_t size) {
    void* ptr = GetFrom(size, bins, 126);
    if (ptr == nullptr) {
        return ptr;
    }
    size_t last_size = node_ptr::GetSize(ptr);
    if (last_size - size > 32) {
        node_ptr::SetMeta(ptr, size + 1);
        void* left = node_ptr::Advance(ptr, size);
        node_ptr::SetMeta(left, last_size - size);
        AddToBin(left);
    }
    return ptr;
}

void AddToFast(void* ptr) {
    AddTo(ptr, fast_bins);
}

void FreeMmap(void* ptr) {
    munmap(node_ptr::Advance(ptr, -8), node_ptr::GetSize(ptr));
}
}  // namespace utils

namespace stdlike {

void* malloc(size_t size) {
    size_t real_size = utils::GetChunkSize(size + 16);
    void* ptr = nullptr;
    if (real_size > constants::kMmapThreshold) {
        return utils::GetMmap(real_size);
    }
    if (real_size <= constants::kFastMax) {
        ptr = utils::GetFast(real_size);
    } else {
        utils::ClearFast();
    }
    if (ptr == nullptr) {
        ptr = utils::GetBin(real_size);
    }
    if (ptr == nullptr) {
        ptr = utils::GetFromHeap(real_size);
    }
    node_ptr::SetOccupied(ptr, 1);
    return ptr;
}

void* calloc(size_t size, size_t amount) {
    void* ptr = malloc(size * amount);
    if (ptr != nullptr) {
        memset(ptr, 0, size * amount);
    }
    return ptr;
}

void* realloc(void* ptr, size_t new_size) {
    if (ptr == nullptr) {
        return malloc(new_size);
    }
    if (!node_ptr::IsValid(ptr)) {
        std::cerr << "Is not valid pointer";
        abort();
    }

    void* new_ptr = nullptr;
    size_t real_size = utils::GetChunkSize(new_size + 16);
    if (node_ptr::IsMmaped(ptr)) {
        new_ptr = utils::GetMremap(ptr, real_size);
    } else {
        if (real_size <= node_ptr::GetSize(ptr)) {
            return ptr;
        }
        ptr = utils::MergeNeighbours(ptr);
        if (real_size <= node_ptr::GetSize(ptr)) {
            new_ptr = ptr;
        } else {
            new_ptr = malloc(new_size);
        }
    }
    if (new_ptr != nullptr) {
        memcpy(new_ptr, ptr, std::min(static_cast<size_t>(node_ptr::GetSize(ptr)) - 16, new_size));
        if (new_ptr != ptr) {
            free(ptr);
        }
    }
    return new_ptr;
}

void free(void* ptr) {
    if (ptr == nullptr) {
        return;
    }
    if (!node_ptr::IsValid(ptr)) {
        std::cerr << "Is not valid pointer\n";
        abort();
    }
    if (node_ptr::IsFree(ptr)) {
        std::cerr << "Double free detected\n";
        abort();
    }

    if (node_ptr::IsMmaped(ptr)) {
        utils::FreeMmap(ptr);
        return;
    }

    size_t size = node_ptr::GetSize(ptr);
    if (size >= constants::kFastConsolidate) {
        utils::ClearFast();
    } else if (size <= constants::kFastMax) {
        utils::AddToFast(ptr);
        return;
    }

    utils::FreePtr(ptr);
}
}  // namespace stdlike
