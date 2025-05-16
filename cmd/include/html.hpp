#ifndef __HTML__HPP
#define __HTML__HPP

#include<iostream>
#include<memory>
#include"ast.hpp"

namespace html {
    extern int precision_level;
    void gen_html(std::ostream& out, const std::shared_ptr<cmd::Node>& node);
    void gen_html_color(std::ostream &out, std::shared_ptr<cmd::Node> &node);
}

#endif