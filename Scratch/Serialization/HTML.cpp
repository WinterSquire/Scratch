#include <iostream>

#include "HTML.hpp"
#include "RawHTML.inl"

int Scratch::createHTMLTemplate(std::ostream &os)
{
    os.write(reinterpret_cast<const char*>(single_html), single_html_len);
    return 0;
}
