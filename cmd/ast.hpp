#ifndef __AST_HPP_X_
#define __AST_HPP_X_

#include"scanner.hpp"
#include<memory>

namespace ast {

    class ASTNode {
    public:
        virtual ~ASTNode() = default;
        virtual void print(std::ostream &os, int indent = 0) const = 0;
    };

    class ASTProgram : public ASTNode {
    public:
        std::vector<std::unique_ptr<ASTNode>> statements;

        void print(std::ostream &os, int indent = 0) const override;
    };

    class ASTStatement : public ASTNode {
    public:
        void print(std::ostream &os, int indent = 0) const override;
    };

    class ASTExpression : public ASTNode {
    public:
        void print(std::ostream &os, int indent = 0) const override;
    };

}

#endif