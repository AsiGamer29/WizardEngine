#include "UUID.h"
#include <sstream>
#include <iomanip>

std::random_device UUID::randomDevice;
std::mt19937 UUID::generator(randomDevice());
std::uniform_int_distribution<uint32_t> UUID::distribution;

UUID::UUID()
    : value(0)
{
}

UUID::UUID(uint32_t value)
    : value(value)
{
}

UUID UUID::Generate()
{
    return UUID(distribution(generator));
}

std::string UUID::ToString() const
{
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << value;
    return ss.str();
}