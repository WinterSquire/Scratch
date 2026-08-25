#pragma once

class CHTMLSerializer
{
public:
    static int process(
        const uint64_t* timestampList, 
        const struct ScratchResultKinetic& data, 
        size_t size, 
        std::ostream& os);
};
