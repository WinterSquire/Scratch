#pragma once

#include <ostream>

class ISerializer
{
public:
    virtual int serialize(
        size_t size,
        struct ScratchParameterKinetic* exp,
        struct ScratchParameterKinetic* con,
        struct ScratchParameterGlobal& parameter, 
        std::ostream& os) = 0;
};