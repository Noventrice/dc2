# dc2
Stack based calculator based on Unix's dc (desk calculator) but with infix notation.

COMMANDS:
+ \# Comments.
+ q quit dc2.
+ p Print top of the stack with a newline appended.
+ t Print the top of the stack as is.
+ d Duplicate the top of the stack.
+ f Show the stack.
+ c Clear the stack.
+ _ Throw away the top of the stack.
+ r Swap the top of the stack with the next value in the stack.
+ s Save the top of the stack to a variable. For example, sx saves to x.
+ l (ell) Load a variable and move it to the top of the stack. For example, lx.
+ [] Strings. For example, [foo]p #prints foo.
+ x Execute string on the top of the stack.
+ ? Get a number from the user (or stdin).

EXAMPLES
+ `(3+4)*5p #35`
+ `2ˆ.5p #Square root of 2.`
+ `(3+?)[3+x=]t_p #Add 3 to your input and print a info about it.`
+ `-3ˆ2p #-9`
+ `-(3d)*p #-9`

+
```
  5     #Add 5 to the top of the stack.
  ()-3p #Subtract 3 from the top of the stack.
```

+ `[foo]p #Prints "foo".`
+ `[[foo]p]x #Also, prints "foo" but uses string evaluation to do it.`

