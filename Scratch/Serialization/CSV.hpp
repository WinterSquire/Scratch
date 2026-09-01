#pragma once

#include "Serialization.hpp"

class CCSVSerializer : public ISerializer
{
public:
    virtual int serialize(
        size_t size,
        const struct ScratchParameterKinetic* exp,
        const struct ScratchParameterKinetic* con,
        const struct ScratchParameterGlobal& parameter, 
        std::ostream& os) override;
};
