#ifndef __HTML__HPP
#define __HTML__HPP

#include<iostream>
#include<memory>
#include"ast.hpp"

namespace html {
    void gen_html(std::ostream& out, const std::shared_ptr<cmd::Node>& node);
}

#endif