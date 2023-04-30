#A graphing calculator written in c.
This application is an ASCII graphing calculator written from scratch.

    To run the application through the terminal (preferred), use the "calculator" command
in the source directory. If you're using VSCode to run the program, open the source directory
and use "./calculator". 

IMPORTANT:
    Make sure that the font size in the terminal is set to the smallest possible size
that still allows you to read the text comfortably, and make sure your terminal is set to fullscreen
mode. This is so that the graphing function appears on your screen properly.
    I have included the msys-2.0.dll, which should be the only requirement to run this program
as a c makefile through the terminal based on trial runs. If any problem is encountered, please
e-mail me at younes.adam23@reavisd220.org, or samakmashwee@gmail.com.

    Once the program is successfully running, you will be prompted to type with a
"$ " indicator, any time you see this indicator, the program is accepting input.

    Input can be accepted in one of two formats:
        1. a mathematical expression.
        2. one of the preset commands for the calculator. (If you would like to know
        more about how to use the calculator, run "/help" after starting the program).

    To end the calculator program properly, run the command "/quit", this will save
all data to the functions file and the window data file, so that when you run the calculator
again, you'll be all set!

LIMITATIONS:
    The program has a few limitations, as it is made completely from scratch...

    FIRST: The largest limitation by far is floating point imprecision. This can be 
    apparent when it comes to complex expressions in the general calculator functionality,
    but is most present when it comes to the integration functionality. The area under 
    the curve will always be slightly inaccurate, although it will be very close to 
    the actual answer. This was largely minimized through the use of (long double) Instead
    of (float) or (double), but it, unfortunately, cannot be counteracted.

    SECOND: The calculator functions as intended regardless of your terminal settings.
    However, there are inherent limitations when it comes to the use of ASCII as the display
    method. While this was intentionally done as a method of stylization, it still makes it so
    that the terminal must be at least as wide as the width of the graphing display. The default
    graphing display width is 200 characters, so as long as the font size of the terminal is
    small enough such that it can fit 200 characters in a row, there will be no problem.

    THIRD: There is one edge case in the parser that has eluded every debugging effort.
    For some unholy reason, the entire parser breaks if an expression starts with something
    in parentheses and is followed by anything. like (x)2, (23)x, or (23)+x. So just
    umm, don't do that. It's a feature.

    FOURTH: Undefined/error handling. There is a very rudimentary error-handling system in 
    place, however, this does not cover the integration between outputs that don't exist.
    so if, for instance, you are integrating under log(x), inputting a lower bound of anything
    less than or equal to 0 will end the program with a segmentation fault, this is because
    it is impossible to integrate between 0 and anything under log(x). This also applies to
    functions with holes and other functions with vertical asymptotes. 



Here's some information about the process of making the calculator!

    The calculator initially started with the vision of making a graphing calculator
that runs in the terminal, with an ASCII display. This was initially inspired by other
ASCII displays that rendered real-time graphics in 3d and 2d. I saw this, and attempted
to implement a similar system in the context of an (x, y) plane.

    The implementation starts with what is essentially an array of pixels, defined by
an x and y coordinate, as well as a single character that would hold what the pixel
actually looks like. This array is what the program uses as the main graphing display.
One of the problems with this method of ASCII pixelization is that it looks absolutely
horrible because each character on the screen takes up so much room.

    To illustrate this problem, I will use the function x^2 as an example.
    
    Initially, the graphing calculator would output the function y = x^2 like this:

        
                                #        |        #
                                 #       |       #
                                  ##     |     ##
                                    ##   |   ##
--------------------------------------#######---------------------------------------
                                         |
                                         |

    Here, you can see that when the y coordinate of any given pixel matched the output
of the function, that pixel would simply display a '#'. While this isn't entirely unre-
-cognizeable as the desired function, it is still clearly ugly and could be a lot better
when it comes to proper function visualization.

    So, clearly, more accuracy was needed, and to achieve this, I had to look at where
the problem lay, which was in the fact that character slots on a computer are taller than
they are wide. This can be inferred because we see multiple hashtags next to each other,
where one should be slightly above the other if the function was completely accurate.
    To rectify this, I had to figure out a way to artificially inflate the resolution of 
the display without actually increasing the number of pixels, and here is where the 
the flexibility of ASCII art offered a potential solution. Instead of setting each pixel's
display to the character that fills the most space, a more gradual and refined line 
could be achieved using characters that take up different places within their character 
slot.

    For instance, when we take a look at the characters "." and "-" side by side, one
is slightly higher than the other. We can utilize this to increase the vertical resolution
of the graphing calculator!
    In order to do this, we need sort of a palette of characters that indicate a gradual
change in y value as opposed to a harsh one. For this palette, the calculator uses the
string "_,.-~*'`". After a little testing and studying ASCII art, I learned that these 
characters were some of the most commonly used in drawing gradual lines.

    After implementing this system of artificially raising the vertical resolution, the
results speak for themselves. Here is the same function x^2 graphed with the new system:

                                ~        |        ~
                                 *       |       *
                                  `.     |     .`
                                    '.   |   .'
--------------------------------------`*~-~*`---------------------------------------
                                         |
                                         |

    Not only does this fix the accuracy problem, it also has the added bonus of that
ASCII art charm that this program was inspired by in the first place!

    To start, I created "graph.h", initially as "graph.c", and only included the 
graphing functionality in the program. On runtime, the program would print out the
graph of a preset function. The only problem with this was that you would have to
change the source code of the program to get it to change the function that it would
graph. I had initially minimized this source code alteration by making the function
that contained the equation separate from everything else, but I still didn't like it.

    I settled on accepting user input for the equation that would be graphed so that
changing the function that was graphed would be easy. The problem then made itself clear.
I would have to accept user input in the form of a mathematical expression written in 
mathematical notation and somehow transform that into an output given the values of x
on the (x, y) plane. 
    Research revealed that solving this problem was going to be more difficult than I'd
initially anticipated, because of the fact that the way humans write and evaluate 
mathematical expressions are very different from the process that computers take to 
writing and evaluating things. For instance, complex expressions like x^(2x+4) are very
easy for someone like you and me to evaluate given the value of x, but for a computer to
look at the same expression and give a result, a few steps must be taken first.
    This is what the file "parser.h" does. Initially, the expression must be converted 
into a form that is easy for a computer to evaluate. One of these formats is called 
Reverse Polish Notation (RPN), or more simply, postfix notation.

    To define postfix notation, let's take a look at the way that people write expressions,
using the example x^(2x+4), this is called infix notation, where the operands are on either
side of an operator. Postfix notation, on the other hand, is when both operands precede the
operator. So x+2 becomes x2+, and our example of x^(2x+4) becomes x2x*4+^. Notice how the 
implied multiplication symbol must be inserted after 2x in the postfix expression, this is
because the implied multiplication symbol between 2 and x must come after the two operands,
meaning it must be explicitly written. Also notice how postfix notation does not use
parentheses; this is important, as parentheses are a big time sink when it comes to 
performing fast text processing like we want. 
    So the first step becomes converting infix notation to postfix notation, which is the 
notation that is far easier to evaluate using code. To do this, each individual token must
be identified in the original notation. The way I chose to do this is similar to the way
that games handle such problems... A state machine! By looping through the original 
string and changing the state of the parser, we can make it perform different actions 
depending on what kind of character it's looking at. 

    Using the state machine in conjunction with a stack, we can convert infix notation to
postfix notation through the following process:

    1. if the current character is an operand, push it to the output.
    2. if the current character is an operator, push it to a stack.
        a.  if the operator at the top of the stack comes later than the current character
        in the order of operations, push the top of the stack to the output.
    3. if the current character is an open parenthesis, push it to the stack.
    4. if the current character is a close parenthesis, push the top of the stack to the 
    output until an open parenthesis is reached.

    Through this process, otherwise known as the shunting-yard algorithm, infix notation is
converted to postfix notation. A similar process can be taken to incorporate trig functionality.

    After this, we have a string that represents an expression in syntax that is incredibly
easy to evaluate given the value of x!

    Notice how this process of converting plain text into processable data is similar to the
process that compilers execute... That's because what we've effectively done is create a 
mathematical expression compiler! With this newfound limitless power, we can conquer the world.

    Okay, not really... But what we can do is finally make a working graphing calculator!!!!!

    "calculator.c" is the file that ties in all of the functionality of the parser and graphing
portions, while at the same time expanding on both.

    It is in this file that I incorporated concepts from calculus, enabling the calculator to
graph the derivative of any given function, as well as integrate under any given curve between
two bounds. These functionalities both use their formal definitions in calculus as a 
framework for their implementations in c.
