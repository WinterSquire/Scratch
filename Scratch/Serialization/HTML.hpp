#pragma once

class CHTMLSerializer
{
public:
    static int process(
        const uint64_t* timestampList, 
        const struct ScratchResultFrame* frames,
        size_t size, 
        const struct ScratchResultKinetic& data, 
        std::ostream& os);
};
