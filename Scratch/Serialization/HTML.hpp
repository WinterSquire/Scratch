#pragma once

#include "Serialization.hpp"

class CHTMLSerializer : public ISerializer
{
public:
    virtual int serialize(
        size_t size,
        struct ScratchParameterKinetic* exp,
        struct ScratchParameterKinetic* con,
        struct ScratchParameterGlobal& parameter, 
        std::ostream& os) override;
};

