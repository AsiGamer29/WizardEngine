#pragma once
#include <nlohmann/json.hpp>

class ComponentSerializer
{
public:
    virtual ~ComponentSerializer() = default;

    virtual nlohmann::json GetProperties() const = 0;
    virtual void SetProperties(const nlohmann::json& props) = 0;
};