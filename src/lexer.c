#include "lexer.h"
#include "debug.h"

void lexer_init(lexer_t* lexer, const char* input, int size) {
    lexer->current = 0;
    lexer->linenumber = 1;
    lexer->buffer = input;
    lexer->size = size;

    lexer->t.type = TK_EOF;
    lexer->t.value = NULL;
    lexer->t.length = 0;
}

void lexer_load(token_t* token, lexer_t* lexer) {
    char character = lexer->buffer[lexer->current];

    if (character == '\0') {
        token->type = TK_EOF;
        token->value = NULL;
        token->length = 0;
        return;
    }

    if (character == '\n') {
        lexer->linenumber++;
        lexer->current++;
        lexer_load(token, lexer);
        return;
    } else if (character == ' ' || character == '\t' || character == '\r') {
        lexer->current++;
        lexer_load(token, lexer);
        return;
    } else if (character >= '0' && character <= '9') {
        token->type = TK_NUMBER;
        token->value = &lexer->buffer[lexer->current];
        token->length = 1;

        while (lexer->current + token->length < lexer->size &&
               lexer->buffer[lexer->current + token->length] >= '0' &&
               lexer->buffer[lexer->current + token->length] <= '9') {
            token->length++;
        }

        lexer->current += token->length;
    } else if (character == '"' || character == '\'') {
        token->type = TK_STRING;
        token->value = &lexer->buffer[lexer->current + 1];
        token->length = 0;

        lexer->current++;
        while (lexer->current < lexer->size && lexer->buffer[lexer->current] != character) {
            token->length++;
            lexer->current++;
        }

        if (lexer->current < lexer->size) {
            lexer->current++;
        }
    } else if (character == '_' || (character >= 'a' && character <= 'z')) {
        token->type = TK_IDENTIFIER;
        token->value = &lexer->buffer[lexer->current];
        token->length = 1;

        while (lexer->current + token->length < lexer->size &&
               (lexer->buffer[lexer->current + token->length] == '_' ||
                (lexer->buffer[lexer->current + token->length] >= 'a' &&
                 lexer->buffer[lexer->current + token->length] <= 'z')
                 || (lexer->buffer[lexer->current + token->length] >= '0' &&
                lexer->buffer[lexer->current + token->length] <= '9'))) {
            token->length++;
        }

        lexer->current += token->length;
    } else if (character >= 'A' && character <= 'Z') {
        switch (character) {
            case 'A': token->type = TK_ASCII; break;
            case 'B': token->type = TK_BLOCK; break;
            case 'C': token->type = TK_CALL; break;
            case 'D': token->type = TK_DUMP; break;
            case 'F': token->type = TK_FALSE; break;
            case 'G': token->type = TK_GET; break;
            case 'I': token->type = TK_IF; break;
            case 'L': token->type = TK_LENGTH; break;
            case 'N': token->type = TK_NULL; break;
            case 'O': token->type = TK_OUTPUT; break;
            case 'P': token->type = TK_PROMPT; break;
            case 'Q': token->type = TK_QUIT; break;
            case 'R': token->type = TK_RANDOM; break;
            case 'S': token->type = TK_SET; break;
            case 'T': token->type = TK_TRUE; break;
            case 'W': token->type = TK_WHILE; break;
            default: panic("Unknown function '%c' at line %d", character, lexer->linenumber);
        };
        token->value = &lexer->buffer[lexer->current];
        token->length = 1;

        while (lexer->current + token->length < lexer->size &&
               ((lexer->buffer[lexer->current + token->length] >= 'A' &&
                 lexer->buffer[lexer->current + token->length] <= 'Z') ||
                lexer->buffer[lexer->current + token->length] == '_')) {
            token->length++;
        }

        lexer->current += token->length;
    } else {
        switch (character) {
            case '+': token->type = TK_PLUS; break;
            case '-': token->type = TK_MINUS; break;
            case '*': token->type = TK_MULTIPLY; break;
            case '/': token->type = TK_DIVIDE; break;
            case '%': token->type = TK_MODULO; break;
            case '^': token->type = TK_POWER; break;
            case '<': token->type = TK_LESS; break;
            case '>': token->type = TK_GREATER; break;
            case '=': token->type = TK_ASSIGN; break;
            case '&': token->type = TK_AND; break;
            case '|': token->type = TK_OR; break;
            case '!': token->type = TK_NOT; break;
            case '?': token->type = TK_EQUAL; break;
            case ';': token->type = TK_EXPR; break;
            case '[': token->type = TK_PRIME; break;
            case ']': token->type = TK_ULTIMATE; break;
            case ',': token->type = TK_BOX; break;
            case '@': token->type = TK_LIST; break;
            default: panic("Unknown character '%c' at line %d", character, lexer->linenumber);
        }

        token->length = 1;
        token->value = &lexer->buffer[lexer->current];
        lexer->current++;
    }
}