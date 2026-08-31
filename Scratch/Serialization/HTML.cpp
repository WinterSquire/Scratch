#include <iostream>
#include "../Scratch.hpp"

#include "HTML.hpp"
#include "JSON.hpp"

#include "../WWW/part1.h"
#include "../WWW/part2.h"

int CHTMLSerializer::serialize(size_t size, ScratchParameterKinetic *exp, ScratchParameterKinetic *con, ScratchParameterGlobal &parameter, std::ostream &os)
{
    os.write((const char*)part1_html, part1_html_len);

    do
    {
        if (size == 0 || exp == NULL)
            break;

        CDataJsSerializer().serialize(size, exp, con, parameter, os);
        os << '\n';

        if (exp->images == NULL)
            break;
        
        CImagesJsSerializer().serialize(size, exp, NULL, parameter, os);
        os << '\n';
    } while (0);

    os.write((const char*)part2_html, part2_html_len);

    return 0;
}
