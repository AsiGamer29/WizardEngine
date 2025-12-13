#pragma once
#include <cstdint>
#include <random>
#include <string>

class UUID
{
public:
    UUID();
    explicit UUID(uint32_t value);

    uint32_t GetValue() const { return value; }
    std::string ToString() const;

    bool operator==(const UUID& other) const { return value == other.value; }
    bool operator!=(const UUID& other) const { return value != other.value; }

    static UUID Generate();

private:
    uint32_t value;
    static std::random_device randomDevice;
    static std::mt19937 generator;
    static std::uniform_int_distribution<uint32_t> distribution;
};