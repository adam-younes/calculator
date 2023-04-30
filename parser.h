#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#ifndef PDEF
#define PDEF static inline
#endif

// the max amount of parentheses that can be present in an expression.
#ifndef MAX_COMPLEXITY
#define MAX_COMPLEXITY 128
#endif

// the maximum length of an expression.
#ifndef MAX_LENGTH
#define MAX_LENGTH 256
#endif

// state machine for the text parser.
typedef enum {
    STATE_par,
    STATE_str,
    STATE_opr,
    STATE_var,
    STATE_con,
    STATE_num,
    STATE_trg,
    STATE_end,
    STATE_err
} p_state;

// type markers for tokens in the makestring.
typedef enum {
    TYPE_par,
    TYPE_opr,
    TYPE_var,
    TYPE_num,
    TYPE_trg,
    TYPE_con
} p_type;

// current parser data.
typedef struct {
    char *input;
    int pos;
    int token_pos;
    int token_cnt;
    char **tokens;
    p_type *types;
    p_state state;
    char *mkstr;
} p_data;

// input definitions for ease of use.
static char *function_shorthand = "sScCtTl";
static char *accepted_inputs = "^+-/*[]{}()1234567890sincotaexlgpi";
static char *accepted_functions[7] = {"sin", "csc", "cos", "sec", "tan", "cot", "log"};

// memory handling.
PDEF p_data *clear_data(p_data *data) {
    free(data);
    return (p_data *) calloc(1, sizeof(p_data));
}

// a function that prints all of the tokens of a parser dataset object
PDEF void print_tokens(p_data *data) {
    for(int i = 0; i < data -> token_cnt; i++)
        printf("%s\n", data -> tokens[i]);
    return;
}

// returns whether or not a character exists in a string.
PDEF bool isin(char c, char *s) {
    int i = 0;
    int length = strlen(s);
    for( ; s[i] != c && i < strlen(s) ; i++) continue;

    return i < strlen(s);
}

// is called when an error occurs. prints error and quits.
PDEF void throw_error(char *s) {
    printf("ERROR: %s\n", s);
    exit(0);
}

// returns the string corresponding to consecutive integers in the input.
void findnum(p_data *data, char *start) {
    int length = 0;

    // if the first character is negative, the number parsing starts at the first character after the '-' symbol.
    if(start[0] == '-')
        length ++;


    while(isin(start[length], "0123456789."))
        length++;


    char *output = (char *) calloc(length, sizeof(char));
    for(int i = 0; i < length; i++)
        output[i] = start[i];

    data -> tokens[data -> token_pos] = output;
    data -> pos += length;
    data -> token_pos++;
}

// inserts a character as a string token into the token array.
void add_ctoken(p_data *data, char c) {
    data -> tokens[data -> token_pos] = (char *) calloc(2, sizeof(char));
    data -> tokens[data -> token_pos][0] = c;
    data -> tokens[data -> token_pos][1] = '\0';
    data -> token_pos++;
    data -> pos++;
    return;
}

// simpifies trig functions to a single character corresponding to that function.
char encode_trig(char *s) {
    for(int i = 0;i < 7; i++)
        for(int j = 0; j < 3; j++)
            if(s[j] == accepted_functions[i][j] && j == 2) return function_shorthand[i];
    return '\0';
}

// removes all whitespace from a string.
PDEF char *eat_whitespace(char *input, int length) {
    int counter = 0;
    char *a_string = calloc(length, sizeof(char));

    for(int i = 0 ; i < length ; i++) {
        if(!isspace(input[i])) {
            a_string[counter] = input[i];
            counter++;
        }
    }

    return a_string;
}

// preprocessing done to input in order to produce a makestring.
void preprocess(p_data *data) {
    int length = strlen(data -> input);
    data -> mkstr = (char *) calloc(length * 2, sizeof(char));

    // encodes trig functions.
    char *b_string = (char *) calloc(length, sizeof(char));
    for(int i = 0, j = 0; j < length; i++, j++) {
        if(isin(data -> input[j], "sctl")) {
            if(encode_trig(data -> input + j) != '\0') {
                b_string[i] = encode_trig(data -> input + j);
                j+=2;
            } else data -> state = STATE_err;
        } else b_string[i] = data -> input[j];
    }

    // insert multiplication symbol where implied by mathematical notation: 2x, xsinx, 3(2-1), etc...
    for(int i = 0, j = 0; i < strlen(b_string); i++, j++) {
        // each conditional here is an edge case that needed to be worked out by hand.
        if((isin(b_string[i+1], "sScCtTl"       ) && !isin(b_string[i]  , "sScCtTl({[/+-*^"        ))
        || (isin(b_string[i+1], "([{"           ) && !isin(b_string[i]  , "([{sScCtTl/+-*^"        ))
        || (isin(b_string[i]  , ")}]"           ) &&  isin(b_string[i+1], "([{xpesScCtTl1234567890"))
        || (isin(b_string[i]  , "0123456789."   ) &&  isin(b_string[i+1], "([{xsScCtTlpe"          ))
        || (isin(b_string[i]  , "xpe"           ) &&  isin(b_string[i+1], "0123456789sScCtTl([{x"  ))) {

            data -> mkstr[j] = b_string[i]; j++;
            data -> mkstr[j] = '*';

        } else { data -> mkstr[j] = b_string[i]; }
    } free(b_string);

}

// returns the current state of the parser based on the type of the character that is being parsed.
PDEF p_state identify(char c) {
    if(isin(c, function_shorthand))             return STATE_trg;
    else if(isin(c, "^+/*-"))                   return STATE_opr;
    else if(isin(c, "pe"))                      return STATE_con;
    else if(isin(c, "()[]{}"))                  return STATE_par;
    else if(c == 'x')                           return STATE_var;
    else if((c >= '0' && c <= '9') || c == '.') return STATE_num;
    else if(c == '\0' || c == '\n')             return STATE_end;
    return STATE_err;
}

// converts string input into tokenized input.
PDEF void tokenize(p_data *data) {

    if(data -> state == STATE_err) {
        throw_error("invalid token");
    } data -> state = STATE_str;

    // state machine loop until end or error encountered.
    char *curstr = data -> mkstr;
    while(data -> state != STATE_err && data -> state != STATE_end) {

        switch(data -> state) {
            // STATE_str is the default state, during which the next character's token is identified.
            case STATE_str:
                data -> state = identify(curstr[0]); break;

            // findnum is called in the case of a number.
            case STATE_num:
                data -> types[data -> token_pos] = TYPE_num;
                findnum(data, curstr);
                data -> state = STATE_str; break;

            // parentheses are treated as single-character tokens.
            case STATE_par:
                data -> types[data -> token_pos] = TYPE_par;
                add_ctoken(data, curstr[0]);
                data -> state = STATE_str; break;

            // variables are treated as single-character tokens.
            case STATE_var:
                data -> types[data -> token_pos] = TYPE_var;
                add_ctoken(data, curstr[0]);
                data -> state = STATE_str; break;

            // if it is a constant then it is treated like a number.
            case STATE_con:
                data -> types[data -> token_pos] = TYPE_con;
                add_ctoken(data, curstr[0]);
                if(curstr[0] == 'p') data -> pos++;
                data -> state = STATE_str; break;


            // trig functions are treated as single-character tokens.
            case STATE_trg:
                data -> types[data -> token_pos] = TYPE_trg;
                add_ctoken(data, curstr[0]);
                data -> state = STATE_str; break;

            // operations are treated as single-character tokens.
            case STATE_opr:
                // if the operation is a negative, then it could potentially be the start of a negative number and not part of an operation.
                if(curstr[0] == '-' && (data -> pos == 0 || !isin(data -> mkstr[ + data -> pos - 1], "1234567890)]}xpe"))) {

                    // this is handled by adding a zero token to the token array and then treating the negative as an operator.
                    data -> types[data -> token_pos] = TYPE_num;
                    add_ctoken(data, '0');
                    data -> pos--;
                    data -> types[data -> token_pos] = TYPE_opr;
                    add_ctoken(data, curstr[0]);
                    data -> state = STATE_str;
                } else {
                    data -> types[data -> token_pos] = TYPE_opr;
                    add_ctoken(data, curstr[0]);
                    data -> state = STATE_str;
                }
                break;

            case STATE_end: return; break;

            case STATE_err: return; break;
        }
        curstr = data -> mkstr + data -> pos;
    }
}

// the order of operations starting from exponents. parentheses will be handled using a stack.
int operation_order(char operation) {
    int output = 0;
    char *order_of_operations = "^*/+-";
    while(operation != order_of_operations[output])
        output++;
    return output;
}

// converts the tokens from infix notation (x+2, 2x^3, sin(cos(x)), etc..) to postfix notation (x2+, 2x3^*, xcs, etc...).
PDEF void infix_to_postfix(p_data *data) {
    int length = strlen(data -> mkstr);
    char **output = (char **) calloc(length, sizeof(char*));
    char **stack  = (char **) calloc(length, sizeof(char*));
    int top = 0, output_position = 0, pcount = 0;

    if(data -> state == STATE_err) {
        throw_error("invalid token");
    } data -> state = STATE_str;

    // loops to the end of token array.
    for( ; data -> token_pos < data -> token_cnt ; data -> token_pos++) {
        switch(data -> types[data -> token_pos]) {

            // numbers are appended immediately to the output.
            case TYPE_num:
                output[output_position] = data -> tokens[data -> token_pos];
                output_position++;
                break;

            // operations are appended to the stack in order of priority.
            case TYPE_opr:
                if(!(stack[top])) {
                    stack[top] = data -> tokens[data -> token_pos];
                } else if(isin(stack[top][0], "[{(")) {
                    top++;
                    stack[top] = data -> tokens[data -> token_pos];
                } else if(operation_order(data -> tokens[data -> token_pos][0]) > operation_order(stack[top][0])) {
                    output[output_position] = stack[top];
                    stack[top] = data -> tokens[data -> token_pos];
                    output_position++;
                } else {
                    top++;
                    stack[top] = data -> tokens[data -> token_pos];
                }
                break;

            // variables are treated as numbers.
            case TYPE_var:
                output[output_position] = data -> tokens[data -> token_pos];
                output_position++;
                break;

            case TYPE_con:
                output[output_position] = data -> tokens[data -> token_pos];
                output_position++;
                break;

            // open parentheses are added to the stack, close parentheses initiate a loop that adds to the output until the open parentheses is found.
            case TYPE_par:
                if(isin(data -> tokens[data -> token_pos][0], "[{(")) {
                    if(stack[top])
                        top++;
                    stack[top] = data -> tokens[data -> token_pos];
                } else {
                    while(!isin(stack[top][0], "[{(")) {
                        output[output_position] = stack[top];
                        top--;
                        output_position++;
                    }
                    top--;

                    if(top >= 0 && isin(stack[top][0], "sScCtTl")) {
                        output[output_position] = stack[top];
                        top--;
                        output_position++;
                    }
                } pcount++;
                break;

            // trig functions are appended to the stack, and effectively treated as operations until the expression is evaluated.
            case TYPE_trg:
                top++;
                stack[top] = data -> tokens[data -> token_pos];
                break;
            }
        }

    // adds whatever operations are in the stack to the output.
    while(top > -1) {
        output[output_position] = stack[top];
        top--;
        output_position++;
    }

    data -> tokens = output;
    data -> token_cnt -= pcount;
    data -> token_pos = 0;
}

// evaluates the postfix string.
PDEF long double evaluate(long double xvalue, p_data *data, long double base) {
    long double *stack = calloc(data -> token_cnt, sizeof(long double));
    bool *states = calloc(data -> token_cnt, sizeof(bool));
    int top = 0;

    // loops through until the end of the token array.
    for( ; data -> token_pos < data -> token_cnt ; data -> token_pos++) {
        // if it is an operand, it gets pushed to the stack.
        if(isin(data -> tokens[data -> token_pos][0], "1234567890.xpe")) {
            if(states[top])
                top++;

            // x is substituted for the x value and pushed to the stack.
            if(data -> tokens[data -> token_pos][0] == 'x') {
                stack[top] = xvalue;
                states[top] = true;
            } else if(data -> tokens[data -> token_pos][0] == 'p') {
                stack[top] = atan(1) * 4;
                states[top] = true;
            } else if(data -> tokens[data -> token_pos][0] == 'e') {
                stack[top] = exp(1);
                states[top] = true;
            } else {
                stack[top] = atof(data -> tokens[data -> token_pos]);
                states[top] = true;
            }
            } else if(isin(data -> tokens[data -> token_pos][0], "+-/^*sScCtTl")) {
                // if it is an operator, the operator is carried out with the top two items of the stack.
                switch(data -> tokens[data -> token_pos][0]) {
                case '+':
                    if(top-1 < 0)
                        throw_error("invalid operation");

                    long double sum = stack[top] + stack[top-1];
                    top--;
                    stack[top] = sum;
                break;

                case '-':
                    if(top-1 < 0)
                        throw_error("invalid operation");

                    long double difference = stack[top-1] - stack[top];
                    top--;
                    stack[top] = difference;
                break;

                case '*':
                    if(top-1 < 0)
                        throw_error("invalid operation");

                    long double product = stack[top] * stack[top-1];
                    top--;
                    stack[top] = product;
                break;

                case '/':
                    if(top-1 < 0)
                        throw_error("invalid operation");

                    long double quotient = stack[top-1] / stack[top];
                    top--;
                    stack[top] = quotient;
                break;

                case '^':
                    if(top-1 < 0)
                        throw_error("invalid operation");

                    long double result = (long double) pow(stack[top-1], stack[top]);
                    top--;
                    stack[top] = result;
                break;

                // if it is a trig function, it is treated like an operator and is performed on the top item of the stack.
                case 's':
                    if(!states[top])
                        throw_error("invalid sin");

                    stack[top] = (long double) sin(stack[top]);
                break;

                case 'S':
                    if(!states[top])
                        throw_error("invalid csc");

                    stack[top] = (long double) (1/sin(stack[top]));
                break;

                case 'c':
                    if(!states[top])
                        throw_error("invalid cos");

                    stack[top] = (long double) cos(stack[top]);
                break;

                case 'C':
                    if(!states[top])
                        throw_error("invalid sec");

                    stack[top] = (long double) (1 / cos(stack[top]));
                break;

                case 't':
                    if(!states[top])
                        throw_error("invalid tan");

                    stack[top] = (long double) tan(stack[top]);
                break;

                case 'T':
                    if(!states[top])
                        throw_error("invalid cot");

                    stack[top] = (long double) (1 / tan(stack[top]));
                break;

                case 'l':
                    if(!states[top])
                    throw_error("invalid log");

                    stack[top] = (long double) (log(stack[top])/log(base));
                break;

            }
        } else {
            throw_error("syntax");
        }
    }

    data -> token_pos = 0;
    return stack[top];
}

// compiles input data into a makestring and tokenizes the makestring
PDEF void compile(p_data *data) {
    preprocess(data);

    data -> tokens = (char **) calloc(MAX_COMPLEXITY, sizeof(char *));
    data -> types = (p_type *) calloc(MAX_COMPLEXITY, sizeof(p_type));
    data -> pos = 0;
    data -> token_pos = 0;

    tokenize(data);

    data -> token_cnt = data -> token_pos;
    data -> pos = 0;
    data -> token_pos = 0;

    infix_to_postfix(data);
}