#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser.h"
#include "graph.h"

// max input length throughout the calculator's runtime.
#ifndef MAX_INPUT_LENGTH
#define MAX_INPUT_LENGTH 256
#endif

// the maximum amount of functions that can be stored at once.
#ifndef MAX_FUNCTIONS
#define MAX_FUNCTIONS 10
#endif

// delta x definition for integration and differentiation.
#ifndef DELTA
#define DELTA (long double) .000001
#endif

// state machine for the main calculator loop.
typedef enum {
    STATE_calc,
    STATE_graph,
    STATE_help,
    STATE_base,
    STATE_derive,
    STATE_x,
    STATE_integrate,
    STATE_window,
    STATE_clear,
    STATE_ftable,
    STATE_add,
    STATE_remove,
    STATE_quit,
    STATE_error
} state;

state calculator_state;

// reads the functions save file and loads it into the given array.
int load_functions(p_data **functions) {
    FILE *functions_file = fopen("functions.txt", "r");
    int i = 0, j = 0;
    char c;
    while((c = fgetc(functions_file)) != EOF) {
        if(c == '\n') {
            functions[i] -> input = eat_whitespace(functions[i] -> input, j);
            compile(functions[i]);
            i++;
            j = 0;
        } else {
            functions[i] -> input[j] = c;
            j++;
        }
    }

    fclose(functions_file);
    return i;
}

// prints the help text from the help file.
void print_help() {
    char c;
    FILE *help = fopen("help.txt", "r");
    if(help == NULL)
        throw_error("error opening file \"help.txt\", please check proper download before proceeding.");
    else
        while((c = fgetc(help)) != EOF)
            printf("%c", c);

    fclose(help);
}

// reads the window data from the save file and returns it as an array of values.
long double *load_window_data() {
    FILE *window_file;
    char *window_data = malloc(1 * MAX_INPUT_LENGTH * 4);
    char *string_data;
    long double *output = malloc(sizeof(long double));

     window_file = fopen("window_data.csv", "r");

    fgets(window_data, (MAX_INPUT_LENGTH*4) + 3, window_file);


    int i = 0;
    // window data is contained in the form of a csv, with the values in order of (xmin, xmax, ymin, ymax)
    string_data = strtok(window_data, ",");
    while(string_data != NULL) {
        output[i] = atof(string_data);
        string_data = strtok(NULL, ",");
        i++;
    }

    free(window_data);
    fclose(window_file);
    return output;
}

// saves the current state of the function table to the functions file.
int save_functions(p_data **functions) {
    FILE *functions_file = fopen("functions.txt", "w");
    char *current_function;
    for(int i = 0 ; i < MAX_FUNCTIONS ; i++) {
        current_function = functions[i] -> input;

        if(strlen(current_function) > 0)
            fprintf(functions_file, "%s\n", current_function);
    }
    char c = fgetc(functions_file);
    if(c == EOF)
        fprintf(functions_file, "\n");
    fclose(functions_file);
    return 0;
}

// saves the current state of the window borders to the window data file
int save_window_data(long double xmin, long double xmax, long double ymin, long double ymax) {
    FILE *window_file = fopen("window_data.csv", "w");
    fprintf(window_file, "%Lf, %Lf, %Lf, %Lf", xmin, xmax, ymin, ymax);
    fclose(window_file);
    return 0;
}

// returns the derivative based on the delta x limit definition of a derivative.
long double derive(long double x_value, p_data *data, long double b) {
    return (evaluate(x_value + DELTA, data, base) - evaluate(x_value, data, b)) / DELTA;
}

// returns the definite integral based on given bounds and the delta x summation definition of an integral.
long double integrate(long double left_bound, long double right_bound, p_data *function) {
    long double x_value = left_bound;
    long double def_int = 0;

    /*
     * ACCURACY LIMITATION
     * -------------------
     *  the accuracy of this calculation is limited by the accuracy of floating point numbers in c.
     *  I have attempted to mitigate this error by modulating the size of the steps taken by the
     *  integration function based on the width of the bounds, but really this is only to maximize
     *  speed rather than accuracy
     */
    long double steps = (right_bound - left_bound) * .00001;
    while(x_value < right_bound) {
        def_int += evaluate(x_value, function, base) * steps;
        x_value += steps;
    }

    return def_int;
}

// prints a formatted function table.
void print_functions(p_data **functions) {
    for(int i = 0 ; i < 10 ; i++)
        printf("y[%i] %s=  %s\n", i+1, i+1 < 10? " " : "", functions[i] -> input? functions[i] -> input : "");

    printf("\n");
}

// returns the index of the first space in a string.
int spaceix(char *input) {
    int i = 0;
    char c;
    while((c = input[i]) != ' ' && i < strlen(input)) i++;
    return i;
}

// breaks command up into detected command line argument and command.
char **parse_command(char *input) {
    char **output = calloc(2, sizeof(char *));
    output[0] = input;
    output[1] = calloc(MAX_INPUT_LENGTH, 1);

    if(spaceix(input) == strlen(input)) {
        output[0] = eat_whitespace(input, strlen(input));
        output[1] = NULL;
        return output;
    }

    input[spaceix(input)] = '\0';
    output[1] = eat_whitespace(&input[spaceix(input)+1], strlen(&input[spaceix(input)+1]));
    return output;
}

// identifies the input and changes the state of the calculator.
char *current_action_id(char *input) {
    if(input[0] == '/') {
        char **commands = parse_command(input);
        if(strcmp(commands[0], "/graph"           ) == 0) calculator_state = STATE_graph;
        else if(strcmp(commands[0], "/help"       ) == 0) calculator_state = STATE_help;
        else if(strcmp(commands[0], "/base"       ) == 0) calculator_state = STATE_base;
        else if(strcmp(commands[0], "/integrate"  ) == 0) calculator_state = STATE_integrate;
        else if(strcmp(commands[0], "/graphdx"    ) == 0) calculator_state = STATE_derive;
        else if(strcmp(commands[0], "/ftable"     ) == 0) calculator_state = STATE_ftable;
        else if(strcmp(commands[0], "/xval"       ) == 0) calculator_state = STATE_x;
        else if(strcmp(commands[0], "/fadd"       ) == 0) calculator_state = STATE_add;
        else if(strcmp(commands[0], "/fremove"    ) == 0) calculator_state = STATE_remove;
        else if(strcmp(commands[0], "/window"     ) == 0) calculator_state = STATE_window;
        else if(strcmp(commands[0], "/quit"       ) == 0) calculator_state = STATE_quit;
        else if(strcmp(commands[0], "/fclear"     ) == 0) calculator_state = STATE_clear;
        else calculator_state = STATE_error;

        if(commands[1] != NULL) {
            char *arg = calloc(strlen(commands[1]), 1);
            for(int i = 0 ; i < strlen(commands[1]) ; i++)
                arg[i] = commands[1][i];
            return arg;
        } else return NULL;
    } else { calculator_state = STATE_calc; return NULL;}
}

int main(void) {
    // general input storage variable for the main loop.
    char *input = calloc(MAX_INPUT_LENGTH, 1);

    long double x_value = 0.0;
    // general dataset for any given expression throughout the calculator's runtime.
    p_data *expression = calloc(1, sizeof(p_data));

    // initialize dataset for functions.
    p_data **functions = calloc(MAX_FUNCTIONS, sizeof(p_data));
    for(int i = 0 ; i < 10 ; i++) {
        functions[i] = calloc(1, sizeof(p_data));
        functions[i] -> input = (char *) calloc(MAX_INPUT_LENGTH, 1);
    } int function_count = load_functions(functions);

    // loads window data into four separate variables for boundaries.
    long double *window_data = load_window_data();
    long double xmin = window_data[0];
    long double xmax = window_data[1];
    long double ymin = window_data[2];
    long double ymax = window_data[3];

    // calculates the width and height of each pixel on the (x, y) plane using the boundaries.
    long double x_steps = ((xmax-xmin) / WINDOW_WIDTH);
    long double y_steps = ((ymax-ymin) / WINDOW_HEIGHT);

    // initializes the display.
    pixel **display = quantify_plane(x_steps, y_steps, xmin, ymax);

    int function_index;

    // general string container for any command line argument.
    char *argument = NULL;

    // main program loop.
    while(true) {

        printf("$ ");
        fgets(input, MAX_INPUT_LENGTH, stdin);
        argument = current_action_id(input);

        switch(calculator_state) {
            // anything that isn't a command is handled by STATE_calc.
            case STATE_calc:
                clear_data(expression);
                expression -> input = calloc(MAX_INPUT_LENGTH, 1);
                strcpy(expression -> input, input);
                expression -> input = eat_whitespace(expression -> input, strlen(input));
                compile(expression);
                printf("\t\t\t%Lf\n", evaluate(x_value, expression, base));
            break;

            // a help command for usability and user education.
            case STATE_help:
                print_help();
            break;

            // graphs the current function table and outputs the graph.
            case STATE_graph:
                draw_plane(display, x_steps, y_steps);

                if(argument != NULL) {
                    clear_data(expression);
                    expression -> input = argument;
                    compile(expression);
                    draw_line(display, &expression, x_steps, y_steps, &evaluate, 1);
                } else draw_line(display, functions, x_steps, y_steps, &evaluate, MAX_FUNCTIONS);
                print_plane(display);
                calculator_state = STATE_calc;
            break;

            // sets the base of log in the calculator.
            case STATE_base:
                if(argument != NULL) {
                    clear_data(expression);
                    expression -> input = argument;
                    compile(expression);
                } else {
                    clear_data(expression);
                    printf("current log() base: %Lf\n", base);
                    printf("new log() base: $ ");
                    expression -> input = calloc(MAX_INPUT_LENGTH, 1);
                    fgets(expression -> input, MAX_INPUT_LENGTH, stdin);
                    expression -> input = eat_whitespace(expression -> input, strlen(expression -> input));
                    compile(expression);
                }

                base = evaluate(x_value, expression, base);
                printf("new log() base set to %Lf\n", base);
            break;

            // prints the current window boundaries
            case STATE_window:
                printf("XMIN: %Lf\n", xmin);
                printf("XMAX: %Lf\n", xmax);
                printf("YMIN: %Lf\n", ymin);
                printf("YMAX: %Lf\n", ymax);

                printf("would you like to change the window bounds? (y or n) $ ");
                fgets(input, MAX_INPUT_LENGTH, stdin);
                if(input[0] == 'y') {
                    printf("which bound would you like to change? (xmin, xmax, ymin, ymax) $ ");
                    fgets(input, MAX_INPUT_LENGTH, stdin);
                    if(strcmp(input, "xmin\n") == 0) {
                        printf("new XMIN: $ ");
                        fgets(input, MAX_INPUT_LENGTH, stdin);
                        xmin = atof(input);
                    } else if(strcmp(input, "xmax\n") == 0) {
                        printf("new XMAX: $ ");
                        fgets(input, MAX_INPUT_LENGTH, stdin);
                        xmax = atof(input);
                    } else if(strcmp(input, "ymin\n") == 0) {
                        printf("new YMIN: $ ");
                        fgets(input, MAX_INPUT_LENGTH, stdin);
                        ymin = atof(input);
                    } else if(strcmp(input, "ymax\n") == 0) {
                        printf("new YMAX: $ ");
                        fgets(input, MAX_INPUT_LENGTH, stdin);
                        ymax = atof(input);
                    } else {
                        calculator_state = STATE_error;
                    }
                    
                    x_steps = ((ymax-ymin) / WINDOW_WIDTH);
                    y_steps = ((ymax-ymin) / WINDOW_HEIGHT);

                    display = quantify_plane(x_steps, y_steps, xmin, ymax);
                }
            break;

            // clear function table.
            case STATE_clear:
                for(int i = 0 ; i < MAX_FUNCTIONS ; i++) {
                    functions[i] = calloc(1, sizeof(p_data));
                    functions[i] -> input = calloc(MAX_INPUT_LENGTH, 1);
                }
            break;

            // change the value of x in general expression evaluation.
            case STATE_x:
                if(argument != NULL) {
                    clear_data(expression);
                    expression -> input = argument;
                    compile(expression);
                } else {
                    clear_data(expression);
                    expression -> input = calloc(MAX_INPUT_LENGTH, 1);
                    printf("current x value for expression evaluation: %Lf\n", x_value);
                    printf("new x value: $ ");
                    fgets(expression -> input, MAX_INPUT_LENGTH, stdin);
                    expression -> input = eat_whitespace(expression -> input, strlen(expression -> input));
                    compile(expression);
                }
                x_value = evaluate(x_value, expression, base);
                printf("new x value set to %Lf\n", x_value);
            break;

            // graphs the derivatives of the functions in the current function table and outputs the graph.
            case STATE_derive:

                draw_plane(display, x_steps, y_steps);
                if(argument != NULL) {
                    clear_data(expression);
                    expression -> input = argument;
                    compile(expression);
                    draw_line(display, &expression, x_steps, y_steps, &derive, 1);
                } else draw_line(display, functions, x_steps, y_steps, &derive, MAX_FUNCTIONS);

                print_plane(display);
            break;

            // graphs the definite integral of a selected function in the function table.
            case STATE_integrate:

                draw_plane(display, x_steps, y_steps);

                // prompts the user for left and right bounds of the integral.
                long double left_bound, right_bound;
                printf("left bound: $ ");
                fgets(input, MAX_INPUT_LENGTH, stdin);
                left_bound = atof(input);
                printf("right bound: $ ");
                fgets(input, MAX_INPUT_LENGTH, stdin);
                right_bound = atof(input);

                if(argument != NULL) {
                    clear_data(expression);
                    expression -> input = argument;
                    compile(expression);
                    shade_graph(display, &expression, x_steps, y_steps, 0, left_bound, right_bound);
                } else {
                    // acquires user input for their desired function to integrate under.
                    if(function_count > 1) {
                        print_functions(functions);
                        printf("which function would you like to integrate under? (1-10)$ ");
                        fgets(input, MAX_INPUT_LENGTH, stdin);
                        function_index = atoi(input) - 1;
                    } else function_index = function_count-1;

                    // error handling
                    if(function_index < 0 || function_index > 9) {
                        printf("ERROR: function does not exist.\n");
                        continue;
                    }

                    // graphs the function with the shading parameters of the draw function enabled, and outputs the graph.
                    shade_graph(display, functions, x_steps, y_steps, function_index, left_bound, right_bound);
                } print_plane(display);

                // prints the AUC.
                printf("area = %Lf\n", integrate(left_bound, right_bound, argument == NULL? functions[function_index] : expression));
            break;

            // displays the function table.
            case STATE_ftable:
                print_functions(functions);
            break;

            // adds a function to the first empty slot in the function table.
            case STATE_add:

                if(function_count == 10) {
                    printf("maximum function count [10] has been reached.\n");
                    printf("which function would you like to replace? (1-10)$ ");
                    fgets(input, MAX_INPUT_LENGTH, stdin);
                    function_index = atoi(input);
                    if(function_index < 1 || function_index > 10) {
                        printf("ERROR: input must be a number between 1 and 10.\n");
                        function_index = function_count;
                        break;
                    }
                }

                function_index = 0;

                // finds the first opening in the function table
                while(function_index < 10 && strlen(functions[function_index] -> input) != 0)
                    function_index++;

                if(argument != NULL) {
                    functions[function_index] -> input = argument;
                } else {

                    // prompts user for function input.
                    printf("new function: ");
                    fgets(functions[function_index] -> input, MAX_INPUT_LENGTH, stdin);

                    // compiles the function and prints the new function table.
                    functions[function_index] -> input = eat_whitespace(functions[function_index] -> input, strlen(functions[function_index] -> input));
                }

                compile(functions[function_index]);
                print_functions(functions);

                function_count++;
            break;

            // removes a desired function from the function table.
            case STATE_remove:

                if(argument != NULL) {
                    function_index = atoi(argument)-1;

                    if(function_index < 0 || function_index > 9) {
                        printf("ERROR: input must be a number between 1 and 10.\n");
                        continue;
                    } else {
                        // clears the function from the function table and prints the updated function table.
                        functions[function_index] = calloc(1, sizeof(p_data));
                        functions[function_index] -> input = calloc(MAX_INPUT_LENGTH, 1);
                        function_count--;

                        print_functions(functions);
                    }
                } else {
                    // prompts the user to select a function to remove.
                    print_functions(functions);
                    printf("which function would you like to remove? (1-10)$ ");
                    fgets(input, MAX_INPUT_LENGTH, stdin);
                    function_index = atoi(input)-1;

                    if(function_index < 0 || function_index > 9) {
                        printf("ERROR: input must be a number between 1 and 10.\n");
                        continue;
                    } else {
                        // clears the function from the function table and prints the updated function table.
                        functions[function_index] = calloc(1, sizeof(p_data));
                        functions[function_index] -> input = calloc(MAX_INPUT_LENGTH, 1);
                        function_count--;

                        print_functions(functions);
                    }
                }
            break;

            // save current runtime data and exit the program.
            case STATE_quit:
                save_functions(functions);
                printf("functions saved successfully.\n");
                save_window_data(xmin, xmax, ymin, ymax);
                printf("window data saved successfully.\n");
                exit(0);
            break;

            // throws error for invalid command.
            case STATE_error:
                printf("ERROR: invalid command, type \"/help\" for a list of commands.\n");
            break;
        }
    }
}