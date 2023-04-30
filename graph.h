#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifndef GDEF
#define GDEF static inline
#endif

// window boundaries/resolution.
#define WINDOW_WIDTH (long double) 200
#define WINDOW_HEIGHT (long double) 100

typedef struct { long double x, y; char display; } pixel;

long double base = 10;

// return whether or not a value is close to another value based off of a certain deviation.
bool close_to(long double x, long double y, long double deviation) { return fabsl(x-y) < deviation; }

GDEF pixel **initialize_display() {
    // initialize display as multidimensional array of pixels.
    pixel **display = (pixel **) calloc(WINDOW_HEIGHT, sizeof(pixel *));
    for(int i = 0; i < WINDOW_HEIGHT; i++)
        display[i] = (pixel *) calloc(WINDOW_WIDTH, sizeof(pixel));
    return display;
}

// converts indices of pixels in a 2d array from absolute positions to positions relative to the origin on the x - y plane.
GDEF pixel **quantify_plane(long double x_steps, long double y_steps, long double xmin, long double ymax) {

    pixel **display = initialize_display();

    for(int y = 0; y < WINDOW_HEIGHT; y++) {
        for(int x = 0; x < WINDOW_WIDTH; x++) {
            *&display[y][x].x = (xmin + (x_steps * x));
            *&display[y][x].y = (ymax - (y_steps * y));
        }
    }
    return display;
}

// returns a different ascii character based on how close a value is to the end of a range of values.
char ycompress(long double num, long double pixel, long double range) {
    char *table = "_,.-~*'`";

    // splits the pixel's height by 1/8
    long double steps = range/8;

    long double goal = num - (pixel - (range/2) );
    int counter = 0;
    long double step = 0;
    while(step < goal) {
        step += steps;
        counter++;
    }
    return table[counter - 1];
}

// prints the display.
GDEF void print_plane(pixel **display) {
    char **output = malloc(sizeof(char*) * WINDOW_HEIGHT + 1);
    for(int i = 0; i < WINDOW_HEIGHT; i++)
        output[i] = malloc(sizeof(char) * WINDOW_WIDTH + 1);

    for(int y = 0; y < WINDOW_HEIGHT; y++) {
        for(int x = 0; x < WINDOW_WIDTH; x++)
            output[y][x] = *&display[y][x].display;

        output[y][(int)WINDOW_WIDTH] = '\0';
    }

    for(int y = 0; y < WINDOW_HEIGHT; y++)
        puts(output[y]);

    for(int i = 0; i <= WINDOW_HEIGHT; i++)
        free(output[i]);
    free(output);
}

// graphs the line and shades under the curve between the given bounds.
GDEF void shade_graph(pixel **display, p_data **data, long double x_steps, long double y_steps, int function_index, long double left_bound, long double right_bound) {
    long double rel_x, rel_y;

    for(int y = 0 ; y < WINDOW_HEIGHT ; y++) {
        for(int x = 0 ; x < WINDOW_WIDTH ; x++) {
            pixel *pixel = &display[y][x];

            rel_x = pixel -> x;
            rel_y = pixel -> y;

            if(strlen(data[function_index] -> input) == 0)
                return;

            long double output = evaluate(rel_x, data[function_index], base);
            if(close_to(output, rel_y, y_steps/2.1))
                pixel -> display = ycompress(output, rel_y, y_steps);
            else if((output < 0? (rel_y < y_steps/2 && rel_y > output) : (rel_y > -y_steps/2 && rel_y < output)) && (rel_x > left_bound && rel_x < right_bound))
                pixel -> display = '#';
        }
    }
}

GDEF void draw_line(pixel **display, p_data **data, long double x_steps, long double y_steps, long double (*eval)(long double, p_data *, long double), int function_count) {
    long double rel_x, rel_y;

    for(int y = 0 ; y < WINDOW_HEIGHT ; y++) {
        for(int x = 0 ; x < WINDOW_WIDTH ; x++) {
            pixel *pixel = &display[y][x];
            rel_x = pixel -> x;
            rel_y = pixel -> y;


            long double output;
            for(int i = 0 ; i < function_count ; i++) {
                if(strlen(data[i] -> input) == 0)
                    continue;
                output = eval(rel_x, data[i], base);
                if(close_to(output, rel_y, y_steps/2.1))
                    pixel -> display = ycompress(output, rel_y, y_steps);
            }

        }
    }
}

// sets the display of every pixel to the correct ascii character.
GDEF void draw_plane(pixel **display, long double x_steps, long double y_steps) {
    long double rel_x, rel_y;
    for(int y = 0; y < WINDOW_HEIGHT; y++) {
        for(int x = 0; x < WINDOW_WIDTH; x++) {
            pixel *pixel = &display[y][x];
            rel_x = pixel -> x;
            rel_y = pixel -> y;

            bool x_zero = close_to(rel_x, 0, x_steps/2.1);
            bool y_zero = close_to(rel_y, 0, y_steps/2.1);
            bool origin = x_zero && y_zero;

            if(origin)
                pixel -> display = '+';
            else if(x_zero)
                pixel -> display = '|';
            else if(y_zero)
                pixel -> display = '-';
            else
                pixel -> display = ' ';
        }
    }
}



GDEF void clear_display(pixel **display) {
    for(int i = 0; i < WINDOW_HEIGHT; i++)
        free(display[i]);
    free(display);
}
