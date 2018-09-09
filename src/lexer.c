
#include "lexer.h"
#include "arraylist.h"
#include "smem.h"
#include "xstring.h"

#define ADD_TOKEN(typex, start, end) {struct token* token = smalloc(sizeof(struct token));\
size_t buflen = (end) - (start);\
token->value = smalloc(buflen + 1);\
memcpy(token->value, (source) + (start), buflen);\
token->value[buflen] = 0;\
token->type = typex;\
token->line = line;\
token->start_col = column - 1;\
token->end_col = column + buflen - 1;\
arraylist_addptr(tokens, token);\
i += buflen - 1;\
column += buflen - 1;}

size_t token_length(char* ptr, size_t len, int isStarting) {
    if (len <= 0) return 0;
    size_t i = 0;
    if (isStarting) {
        char c = ptr[0];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_')) {
            return 0;
        }
        i++;
    }
    for (; i < len; i++) {
        char c = ptr[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')) {
            return i;
        }
    }
    return len;
}

size_t string_length(char* ptr, size_t len, char term) {
    int escaped = 0;
    for (size_t i = 0; i < len; i++) {
        if (escaped) {
            escaped = 0;
            continue;
        }
        char c = ptr[i];
        if (c == '\\') {
            escaped = 1;
            continue;
        } else if (c == term) {
            return i;
        }
    }
    return len;
}

void processString(char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (str[len] == 0) str[len] = '\n';
    }
}

void tokenize(char* source, size_t src_len, struct arraylist* tokens) {
    size_t column = 1;
    size_t line = 1;
    size_t token_col_start = -1;
    size_t token_line_start = -1;
    for (size_t i = 0; i < src_len; i++) {
        if (source[i] == '\n' || source[i] == 0) {
            line++;
            column = 1;
        } else {
            column++;
        }
        
        uint16_t single_type = TOKEN_UNKNOWN;
        size_t str_len = 0;
        switch (source[i]) {
            case ' ':
            case '\n':
            case 0:
            case '\t':
            case '\v':
            case '\f':
            case '\r':
            break;
            case '"':;
            str_len = string_length(source + i + 1, src_len - i - 1, '"');
            ADD_TOKEN(TOKEN_STRING_LIT; processString(token->value, buflen - 1), i + 1, i + 1 + str_len);
            i += 2;
            column += 2;
            break;
            case '\'':;
            str_len = string_length(source + i + 1, src_len - i - 1, '\'');
            ADD_TOKEN(TOKEN_CHAR_LIT; processString(token->value, buflen - 1), i + 1, i + 1 + str_len);
            i += 2;
            column += 2;
            break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '.':
            if (source[i] == '.' && (i + 1 >= src_len || source[i + 1] < '0' || source[i + 1] > '9')) {
                if (i + 2 < src_len && source[i + 1] == '.' && source[i + 2] == '.') {
                    ADD_TOKEN(TOKEN_ELLIPSIS, i, i + 3);
                } else {
                    ADD_TOKEN(TOKEN_PERIOD, i, i + 1);
                }
                break;
            }
            size_t x = i + 1;
            int hex_escape = 0;
            int binary_escape = 0;
            int has_decimal = source[i] == '.';
            for (; x < src_len; x++) {
                char c2 = source[x];
                if (source[i] == '0' && x == i + 1 && c2 == 'x') {
                    hex_escape = 1;
                } else if (source[i] == '0' && x == i + 1 && c2 == 'b') {
                    binary_escape = 1;
                } else if (!hex_escape && !binary_escape && !has_decimal && c2 == '.') {
                    has_decimal = 1;
                } else if (!hex_escape && !binary_escape && (c2 < '0' || c2 > '9')) {
                    break;
                } else if (hex_escape && !((c2 >= '0' && c2 <= '9') || (c2 >= 'A' && c2 <= 'F') || (c2 >= 'a' && c2 <= 'f'))) {
                    break;
                } else if (binary_escape && c2 != '0' && c2 != '1') {
                    break;
                }
            }
            ADD_TOKEN(has_decimal ? TOKEN_NUMERIC_DECIMAL_LIT : TOKEN_NUMERIC_LIT, i, x);
            break;
            case '{':
            single_type = TOKEN_LCURLY;
            goto single_nolookahead;
            case '}':
            single_type = TOKEN_RCURLY;
            goto single_nolookahead;
            case '[':
            single_type = TOKEN_LBRACK;
            goto single_nolookahead;
            case ']':
            single_type = TOKEN_RBRACK;
            goto single_nolookahead;
            case '(':
            single_type = TOKEN_LPAREN;
            goto single_nolookahead;
            case ')':
            single_type = TOKEN_RPAREN;
            goto single_nolookahead;
            case ';':
            single_type = TOKEN_SEMICOLON;
            goto single_nolookahead;
            case ':':
            single_type = TOKEN_COLON;
            goto single_nolookahead;
            case ',':
            single_type = TOKEN_COMMA;
            goto single_nolookahead;
            case '?':
            single_type = TOKEN_QMARK;
            single_nolookahead:
            ADD_TOKEN(single_type, i, i + 1);
            break;
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '&':
            case '|':
            case '^':
            case '!':
            case '~':
            case '=':
            case '<':
            case '>':
            if (i < src_len - 1 && source[i + 1] == '=') {
                if (i < src_len - 2 && source[i + 2] == '=') {
                    switch (source[i]) {
                        case '+':
                        single_type = TOKEN_PLUS_EQUALS_PRE;
                        break;
                        case '-':
                        single_type = TOKEN_MINUS_EQUALS_PRE;
                        break;
                        case '*':
                        single_type = TOKEN_MUL_EQUALS_PRE;
                        break;
                        case '/':
                        single_type = TOKEN_DIVIDE_EQUALS_PRE;
                        break;
                        case '%':
                        single_type = TOKEN_MODULUS_EQUALS_PRE;
                        break;
                        case '&':
                        single_type = TOKEN_AND_EQUALS_PRE;
                        break;
                        case '|':
                        single_type = TOKEN_OR_EQUALS_PRE;
                        break;
                        case '^':
                        single_type = TOKEN_XOR_EQUALS_PRE;
                        break;
                        case '!':
                        single_type = TOKEN_VAL_NOT_EQUAL;
                        break;
                        case '=':
                        single_type = TOKEN_VAL_EQUAL;
                        break;
                        case '<':
                        case '>':
                        single_type = TOKEN_UNKNOWN;
                        break;
                        case '~':
                        single_type = TOKEN_NOT_EQUALS_PRE;
                    }
                    ADD_TOKEN(single_type, i, i + 3);
                } else {
                    switch (source[i]) {
                        case '+':
                        single_type = TOKEN_PLUS_EQUALS;
                        break;
                        case '-':
                        single_type = TOKEN_MINUS_EQUALS;
                        break;
                        case '*':
                        single_type = TOKEN_MUL_EQUALS;
                        break;
                        case '/':
                        single_type = TOKEN_DIVIDE_EQUALS;
                        break;
                        case '%':
                        single_type = TOKEN_MODULUS_EQUALS;
                        break;
                        case '&':
                        single_type = TOKEN_AND_EQUALS;
                        break;
                        case '|':
                        single_type = TOKEN_OR_EQUALS;
                        break;
                        case '^':
                        single_type = TOKEN_XOR_EQUALS;
                        break;
                        case '!':
                        break;
                        break;
                        case '=':
                        single_type = TOKEN_EQUAL;
                        break;
                        case '<':
                        single_type = TOKEN_LTE;
                        break;
                        case '>':
                        single_type = TOKEN_GTE;
                        break;
                        case '~':
                        single_type = TOKEN_NOT_EQUALS;
                    }
                    ADD_TOKEN(single_type, i, i + 2);
                }
            } else if (i < src_len - 1 && source[i] == '=' && source[i + 1] == '>') {
                ADD_TOKEN(TOKEN_ARROW, i, i + 2);
            } else if (i < src_len - 1 && (source[i] == '/' || source[i] == '&' || source[i] == '|' || source[i] == '*' || source[i] == '<' || source[i] == '>') && source[i + 1] == source[i]) {
                int eqc = 0;
                if (source[i] != '/' && i < src_len - 2 && source[i + 2] == '=') {
                    eqc++;
                    if (i < src_len - 3 && source[i + 3] == '=') {
                        eqc++;
                    }
                }
                uint16_t type = TOKEN_UNKNOWN;
                switch (source[i]) {
                    case '&':
                    type = eqc == 0 ? TOKEN_LAND : (eqc == 1 ? TOKEN_LAND_EQUALS : TOKEN_LAND_EQUALS_PRE);
                    break;
                    case '|':
                    type = eqc == 0 ? TOKEN_LOR : (eqc == 1 ? TOKEN_LOR_EQUALS : TOKEN_LOR_EQUALS_PRE);
                    break;
                    case '*':
                    type = eqc == 0 ? TOKEN_EXP : (eqc == 1 ? TOKEN_EXP_EQUALS : TOKEN_EXP_EQUALS_PRE);
                    break;
                    case '<':
                    type = eqc == 0 ? TOKEN_LSH : (eqc == 1 ? TOKEN_LSH_EQUALS : TOKEN_LSH_EQUALS_PRE);
                    break;
                    case '>':
                    type = eqc == 0 ? TOKEN_RSH : (eqc == 1 ? TOKEN_RSH_EQUALS : TOKEN_RSH_EQUALS_PRE);
                    break;
                    case '/':;
                    i += strlen(source + i) - 1;
                    type = 0;
                }
                if (type != 0) ADD_TOKEN(type, i, i + 2 + eqc);
            } else if (i < src_len - 1 && (source[i] == '+' || source[i == '-']) && source[i + 1] == source[i]) {
                ADD_TOKEN(source[i] == '+' ? TOKEN_INC : TOKEN_DEC, i, i + 2);
            } else {
                switch (source[i]) {
                    case '+':
                    single_type = TOKEN_PLUS;
                    break;
                    case '-':
                    single_type = TOKEN_MINUS;
                    break;
                    case '*':
                    single_type = TOKEN_MUL;
                    break;
                    case '/':
                    single_type = TOKEN_DIVIDE;
                    break;
                    case '%':
                    single_type = TOKEN_MODULUS;
                    break;
                    case '&':
                    single_type = TOKEN_AND;
                    break;
                    case '|':
                    single_type = TOKEN_OR;
                    break;
                    case '^':
                    single_type = TOKEN_XOR;
                    break;
                    case '!':
                    single_type = TOKEN_LNOT;
                    break;
                    case '=':
                    single_type = TOKEN_EQUALS;
                    break;
                    case '<':
                    single_type = TOKEN_LT;
                    break;
                    case '>':
                    single_type = TOKEN_GT;
                    break;
                    case '~':
                    single_type = TOKEN_NOT;
                }
                ADD_TOKEN(single_type, i, i + 1);
            }
            break;
            default:;
            size_t token_len = token_length(source + i, src_len - i, 1);
            switch (source[i]) {
                case 'a':
                if (token_len == 5 && str_startsWithCase(source + i + 1, "sync")) {
                    ADD_TOKEN(TOKEN_ASYNC, i, i + 5);
                    break;
                }
                goto main_default;
                case 'b':
                if (token_len == 5 && str_startsWithCase(source + i + 1, "reak")) {
                    ADD_TOKEN(TOKEN_BREAK, i, i + 5);
                    break;
                }
                goto main_default;
                case 'c':
                if (token_len == 5 && str_startsWithCase(source + i + 1, "lass")) {
                    ADD_TOKEN(TOKEN_CLASS, i, i + 5);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "sig")) {
                    ADD_TOKEN(TOKEN_CSIG, i, i + 4);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "ase")) {
                    ADD_TOKEN(TOKEN_CASE, i, i + 4);
                    break;
                } else if (token_len == 8 && str_startsWithCase(source + i + 1, "ontinue")) {
                    ADD_TOKEN(TOKEN_CONTINUE, i, i + 8);
                    break;
                } else if (token_len == 5 && str_startsWithCase(source + i + 1, "atch")) {
                    ADD_TOKEN(TOKEN_CATCH, i, i + 5);
                    break;
                } else if (token_len == 5 && str_startsWithCase(source + i + 1, "onst")) {
                    ADD_TOKEN(TOKEN_CONST, i, i + 5);
                    break;
                }
                goto main_default;
                case 'd':
                if (token_len == 6 && str_startsWithCase(source + i + 1, "efault")) {
                    ADD_TOKEN(TOKEN_DEFAULT, i, i + 6);
                    break;
                }
                goto main_default;
                case 'e':
                if (token_len == 4 && str_startsWithCase(source + i + 1, "lse")) {
                    ADD_TOKEN(TOKEN_ELSE, i, i + 4);
                    break;
                }
                goto main_default;
                case 'f':
                if (token_len == 3 && str_startsWithCase(source + i + 1, "or")) {
                    ADD_TOKEN(TOKEN_FOR, i, i + 3);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "unc")) {
                    ADD_TOKEN(TOKEN_FUNC, i, i + 4);
                    break;
                } else if (token_len == 7 && str_startsWithCase(source + i + 1, "inally")) {
                    ADD_TOKEN(TOKEN_FUNC, i, i + 7);
                    break;
                }
                goto main_default;
                case 'g':
                if (token_len == 4 && str_startsWithCase(source + i + 1, "oto")) {
                    ADD_TOKEN(TOKEN_GOTO, i, i + 4);
                    break;
                }
                goto main_default;
                case 'i':
                if (token_len == 2 && str_startsWithCase(source + i + 1, "f")) {
                    ADD_TOKEN(TOKEN_IF, i, i + 2);
                    break;
                } else if (token_len == 5 && str_startsWithCase(source + i + 1, "face")) {
                    ADD_TOKEN(TOKEN_IFACE, i, i + 5);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "nst")) {
                    ADD_TOKEN(TOKEN_INST, i, i + 4);
                    break;
                } else if (token_len == 6 && str_startsWithCase(source + i + 1, "mport")) {
                    ADD_TOKEN(TOKEN_IMPORT, i, i + 6);
                    break;
                }
                goto main_default;
                case 'm':
                if (token_len == 6 && str_startsWithCase(source + i + 1, "odule")) {
                    ADD_TOKEN(TOKEN_MODULE, i, i + 6);
                    break;
                }
                goto main_default;
                case 'n':
                if (token_len == 3 && str_startsWithCase(source + i + 1, "ew")) {
                    ADD_TOKEN(TOKEN_NEW, i, i + 3);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "ull")) {
                    ADD_TOKEN(TOKEN_NULL, i, i + 4);
                    break;
                }
                goto main_default;
                case 'p':
                if (token_len == 4 && str_startsWithCase(source + i + 1, "riv")) {
                    ADD_TOKEN(TOKEN_PRIV, i, i + 4);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "rot")) {
                    ADD_TOKEN(TOKEN_PROT, i, i + 4);
                    break;
                } else if (token_len == 3 && str_startsWithCase(source + i + 1, "ub")) {
                    ADD_TOKEN(TOKEN_PUB, i, i + 3);
                    break;
                } else if (token_len == 9 && str_startsWithCase(source + i + 1, "rotofunc")) {
                    ADD_TOKEN(TOKEN_PROTOFUNC, i, i + 9);
                    break;
                } else if (token_len == 4 && str_startsWithCase(source + i + 1, "ure")) {
                    ADD_TOKEN(TOKEN_PURE, i, i + 4);
                    break;
                }
                goto main_default;
                case 'r':
                if (token_len == 3 && str_startsWithCase(source + i + 1, "et")) {
                    ADD_TOKEN(TOKEN_RET, i, i + 6);
                    break;
                }
                goto main_default;
                case 's':
                if (token_len == 6 && str_startsWithCase(source + i + 1, "witch")) {
                    ADD_TOKEN(TOKEN_SWITCH, i, i + 6);
                    break;
                } else if (token_len == 5 && str_startsWithCase(source + i + 1, "ynch")) {
                    ADD_TOKEN(TOKEN_SYNCH, i, i + 5);
                    break;
                } else if (token_len == 6 && str_startsWithCase(source + i + 1, "tatic")) {
                    ADD_TOKEN(TOKEN_STATIC, i, i + 6);
                    break;
                }
                goto main_default;
                case 't':
                if (token_len == 3 && str_startsWithCase(source + i + 1, "ry")) {
                    ADD_TOKEN(TOKEN_TRY, i, i + 3);
                    break;
                } else if (token_len == 5 && str_startsWithCase(source + i + 1, "hrow")) {
                    ADD_TOKEN(TOKEN_THROW, i, i + 5);
                    break;
                }
                goto main_default;
                case 'v':
                if (token_len == 4 && str_startsWithCase(source + i + 1, "irt")) {
                    ADD_TOKEN(TOKEN_VIRT, i, i + 4);
                    break;
                }
                goto main_default;
                case 'w':
                if (token_len == 5 && str_startsWithCase(source + i + 1, "hile")) {
                    ADD_TOKEN(TOKEN_WHILE, i, i + 5);
                    break;
                }
                default:;
                main_default:;
                if (token_len > 0) {
                    ADD_TOKEN(TOKEN_IDENTIFIER, i, i + token_len);
                } else {
                    ADD_TOKEN(TOKEN_UNKNOWN, i, i + 1);
                }
            }
        }
    }
}