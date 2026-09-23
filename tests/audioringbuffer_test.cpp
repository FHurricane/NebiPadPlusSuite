#include "audioringbuffer.h"

#include <algorithm>
#include <cstring>
#include <thread>
#include <vector>

int main()
{
    AudioRingBuffer ring(1000);
    char data[400];
    for (int i = 0; i < 400; ++i) data[i] = char(i % 127);
    if (!ring.push(data, 400) || !ring.push(data, 400)) return 1;
    if (ring.push(data, 400)) return 2;
    char output[1200] = {};
    if (ring.pop(output, 500) != 500 || std::memcmp(output, data, 400) != 0 ||
        std::memcmp(output + 400, data, 100) != 0) return 3;
    if (!ring.push(data, 400)) return 4;
    if (ring.pop(output, 700) != 700 ||
        std::memcmp(output, data + 100, 300) != 0 ||
        std::memcmp(output + 300, data, 400) != 0) return 5;
    if (ring.available() != 0) return 6;
    AudioRingBuffer concurrent(4096);
    std::vector<char> expected(256000);
    std::vector<char> actual(expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) expected[i] = char(i % 251);
    std::thread producer([&] {
        for (std::size_t i = 0; i < expected.size(); i += 256) {
            while (!concurrent.push(expected.data() + i, 256)) std::this_thread::yield();
        }
    });
    std::size_t received = 0;
    while (received < actual.size()) {
        const std::size_t count = concurrent.pop(actual.data() + received,
            std::min<std::size_t>(actual.size() - received, 512));
        if (count == 0) std::this_thread::yield();
        received += count;
    }
    producer.join();
    if (actual != expected || concurrent.available() != 0) return 7;
    return 0;
}
