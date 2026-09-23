#pragma once

#include <atomic>
#include <cstddef>
#include <vector>

class AudioRingBuffer
{
public:
    explicit AudioRingBuffer(std::size_t capacity);
    bool push(const char *data, std::size_t size) noexcept;
    std::size_t pop(char *destination, std::size_t maximum) noexcept;
    std::size_t available() const noexcept;

private:
    std::vector<char> buffer_;
    std::atomic<std::size_t> write_{0};
    std::atomic<std::size_t> read_{0};
};
