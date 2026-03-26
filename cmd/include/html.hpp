#ifndef __HTML__HPP
#define __HTML__HPP

#include "ast.hpp"
#include <iostream>
#include <memory>

namespace html {
    extern int precision_level;
    void gen_html(std::ostream &out, const std::shared_ptr<cmd::Node> &node);
    void gen_html_color(std::ostream &out, std::shared_ptr<cmd::Node> &node);
} // namespace html

#endif