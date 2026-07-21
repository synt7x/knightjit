#include "parser.hpp"
#include "arena.hpp"

parser::node_id parser::nullary() {
    lexer::token token = lex.consume();

    return nodes.create(node {
        token.type,
        frog::token_to_span(token),
        {},
    });
}

parser::node_id parser::unary() {
    lexer::token token = lex.consume();
    node_id operand = expression();

    return nodes.create(node {
        token.type,
        frog::token_to_span(token),
        { operand },
    });
}

parser::node_id parser::binary() {
    lexer::token token = lex.consume();
    node_id left = expression();
    node_id right = expression();

    return nodes.create(node {
        token.type,
        frog::token_to_span(token),
        { left, right },
    });
}

parser::node_id parser::ternary() {
    lexer::token token = lex.consume();
    node_id first = expression();
    node_id second = expression();
    node_id third = expression();

    return nodes.create(node {
        token.type,
        frog::token_to_span(token),
        { first, second, third },
    });
}

parser::node_id parser::quaternary() {
    lexer::token token = lex.consume();
    node_id first = expression();
    node_id second = expression();
    node_id third = expression();
    node_id fourth = expression();

    return nodes.create(node {
        token.type,
        frog::token_to_span(token),
        { first, second, third, fourth },
    });
}

parser::node_id parser::expression() {
    lexer::token token = lex.peek();
    
    switch (token.type) {
        case node_type::NONE:
            frog::croak(lex.src, frog::diagnostic {
                frog::level::error,
                frog::message::expected_expression,
                frog::token_to_span(token)
            });
            break;
        case node_type::ERROR:
            frog::croak(lex.src, frog::diagnostic {
                frog::level::error,
                frog::message::unexpected_token,
                frog::token_to_span(token)
            });
            break;
        case node_type::STRING: case node_type::NUMBER:
        case node_type::VARIABLE: case node_type::TRUE:
        case node_type::FALSE: case node_type::NIL:
        case node_type::ARRAY: case node_type::PROMPT:
        case node_type::RANDOM: case node_type::ARGS:
            return nullary();
        case node_type::CALL: case node_type::DUMP:
        case node_type::OUTPUT: case node_type::LENGTH:
        case node_type::NOT: case node_type::NEGATE:
        case node_type::ASCII: case node_type::BOX:
        case node_type::HEAD: case node_type::TAIL:
        case node_type::QUIT: case node_type::BLOCK:
            return unary();
        case node_type::ADD: case node_type::SUBTRACT:
        case node_type::MULTIPLY: case node_type::DIVIDE:
        case node_type::MOD: case node_type::POWER:
        case node_type::GREATER: case node_type::LESS:
        case node_type::COMPARE: case node_type::AND:
        case node_type::OR: case node_type::EXPR:
        case node_type::EQUAL: case node_type::WHILE:
            return binary();
        case node_type::IF: case node_type::GET:
            return ternary();
        case node_type::SET:
            return quaternary();
        default:
            frog::croak(lex.src, frog::diagnostic {
                frog::level::error,
                frog::message::unexpected_token,
                frog::token_to_span(token)
            });
            break;
    }

    failed = true;

    return nodes.create(node {
        node_type::ERROR,
        frog::token_to_span(lex.consume()),
        {},
    });
}

parser::ast parser::parse() {
    parser::node_id root = expression();

    if (lex.peek().type != lexer::token_type::NONE) {
        frog::croak(lex.src, frog::diagnostic {
            frog::level::error,
            frog::message::token_at_end,
            frog::span { lex.peek().start, lex.peek().length }
        });

        failed = true;
    }

    return nodes.at(root);
}