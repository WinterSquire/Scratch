#include <iostream>
#include "../Scratch.hpp"

#include "HTML.hpp"
#include "JSON.hpp"

#include "../WWW/head_html.h"
#include "../WWW/body_html.h"
#include "../WWW/foot_html.h"
#include "../WWW/index_css.h"

#include "../WWW/echarts_min_js.h"
#include "../WWW/openseadragon_min_js.h"
#include "../WWW/scratch_js.h"
#include "../WWW/main_js.h"

const char importExternalScript[] = R"(
    <script src="data.js"></script>
    <script src="images.js"></script>
    <script>
)";

const char tagScript[] = R"(
    <script>
)";

int CHTMLSerializer::serialize(size_t size, const ScratchParameterKinetic *exp, const ScratchParameterKinetic *con, const ScratchParameterGlobal &parameter, std::ostream &os)
{
    os.write((const char*)head_html, head_html_len);
    os.write((const char*)index_css, index_css_len);
    os.write((const char*)body_html, body_html_len);

    do
    {
        if (size == 0 || exp == NULL)
        {
            os.write(importExternalScript, sizeof(importExternalScript)-1);
            break;
        }

        os.write(tagScript, sizeof(tagScript));

        CDataJsonSerializer().serialize(size, exp, con, parameter, os);
        os << '\n';

        if (exp->images == NULL)
            break;
        
        CImagesJsonSerializer().serialize(size, exp, NULL, parameter, os);
        os << '\n';
    } while (0);

    os.write((const char*)echarts_min_js, echarts_min_js_len);
    os << '\n';
    os.write((const char*)openseadragon_min_js, openseadragon_min_js_len);
    os << '\n';
    os.write((const char*)scratch_js, scratch_js_len);
    os << '\n';
    os.write((const char*)main_js, main_js_len);
    os << '\n';
    os.write((const char*)foot_html, foot_html_len);

    return 0;
}
