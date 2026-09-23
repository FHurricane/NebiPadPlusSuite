#include "audioringbuffer.h"

#include <algorithm>
#include <cstring>

AudioRingBuffer::AudioRingBuffer(std::size_t capacity) : buffer_(std::max<std::size_t>(capacity, 1)) {}

bool AudioRingBuffer::push(const char *data, std::size_t size) noexcept
{
    if (size == 0) return true;
    const std::size_t write = write_.load(std::memory_order_relaxed);
    const std::size_t read = read_.load(std::memory_order_acquire);
    if (size > buffer_.size() - (write - read)) return false;
    const std::size_t offset = write % buffer_.size();
    const std::size_t first = std::min(size, buffer_.size() - offset);
    std::memcpy(buffer_.data() + offset, data, first);
    if (first < size) std::memcpy(buffer_.data(), data + first, size - first);
    write_.store(write + size, std::memory_order_release);
    return true;
}

std::size_t AudioRingBuffer::pop(char *destination, std::size_t maximum) noexcept
{
    const std::size_t read = read_.load(std::memory_order_relaxed);
    const std::size_t write = write_.load(std::memory_order_acquire);
    const std::size_t size = std::min(maximum, write - read);
    if (size == 0) return 0;
    const std::size_t offset = read % buffer_.size();
    const std::size_t first = std::min(size, buffer_.size() - offset);
    std::memcpy(destination, buffer_.data() + offset, first);
    if (first < size) std::memcpy(destination + first, buffer_.data(), size - first);
    read_.store(read + size, std::memory_order_release);
    return size;
}

std::size_t AudioRingBuffer::available() const noexcept
{
    return write_.load(std::memory_order_acquire) - read_.load(std::memory_order_acquire);
}
