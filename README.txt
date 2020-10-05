pong.c
======

A clone of Pong written in less than 500 lines of minimalistic C.

Setup
=====

Windows
-------

To compile this on a Windows system, visit the MSYS2 website
(https://www.msys2.org/) and follow the instructions in the
"Installation" section of the main page.

Once it is installed, open the MSYS2 MinGW64 terminal then run the
following commands (without the '$'):

	$ pacman -S base-devel
	$ pacman -S mingw-w64-x86_64-gcc
	$ pacman -S mingw-w64-x86_64-glfw
	$ pacman -S mingw-w64-x86_64-mesa
	$ pacman -S mingw-w64-x86_64-libpng
	$ pacman -S mingw-w64-x86_64-zlib

Linux / *BSD / etc
------------------

Using your operating system's package manager, install the following
packages:

	- gcc
	- make
	- GLFW3
	- GLU
	- libpng
	- zlib

The packages `gcc` and `make` are likely to already be installed on
your system.  If not, they will usually be brought in by a meta-package
named like `build-essential` or `base-devel`.

In most distributions of Linux, you will need to install the
development versions of the packages `libglfw3`, `libglu`, `libpng`,
and `zlib`.  Most of the time, these packages will be suffixed with
`-dev` or `-devel`.

Compiling
=========

Windows (using MSYS2/MinGW)
---------------------------

Run the MinGW 64-bit terminal instead of the MSYS2
terminal.  Use "cd" to navigate back to where this project was
downloaded to, then run:

	$ make -f Makefile.mingw
	$ ./pong.exe

Linux
-----

Once you've installed the dependencies using your distro's package
manager, these commands will build then run the game.

	$ make
  $ ./pong

Users may also install the program to their PATH by running the command
`make install` as root.

	# make install

By default, the program is installed to `/usr/local/bin`.  To install
in a different location, set the value of the PREFIX variable when
running `make install` e.g.:

	# PREFIX=$HOME/bin make install

...which installs the game to the "bin" directory for the current user.

Playing
=======

Run the game with the `./pong` command (`./pong.exe` in Windows).

Start the game with the Enter key once it is running, control the left
paddle using the Up and Down arrow keys, and one may quit the game with
Escape.

Explanation of Code
===================

This program is written is such a way that it uses a large
amount of C's feature set.  This includes features that may be
beginner-unfriendly.  Additionally, someone unfamiliar C may find it
difficult to follow along.

Overall Structure
-----------------

The source code's structure is chosen to maintain structure
without needing multiple source files.  All custom data types are
defined first, followed by function definitons, then global variable
declaration, and finally each of the program's functions are defined
in the same order as they are declared.  Lastly, the program's main()
function is defined at the very end of the source file.  For a top-down
overview of the program, my recommendation would be to start with the
main() function, and follow along there, scrolling back up to function
definitions for more detailed understanding of what is being done.

Code from Libraries
-------------------

This program depends on three main libraries: GLFW, OpenGL, and libpng.

Functions from GLFW are in camelCase prefixed by "glfw" e.g.
"glfwWindowShouldClose()".  Functions from OpenGL are also in
camelCase, prefixed just by "gl" e.g. "glBegin()", "glVertex2f()".
Functions and data types from libpng are in snake_case, prefixed by
"png_".

Pointers
--------

Besides those pointers needed for the libraries GLFW and libpng, pong.c
also passes pointers into one function call: pongobject_update().
This is intended to show that pointers are excellent for functions that
are meant to update data in-place.  Though all instances of PongObject
are global variables, the approach used by pongobject_update()
is meant to demonstrate that not all functions in an application
need to explicitly name globals in their definitons.  This allows
future changes to variable locations to be made much more easily,
because this update function doesn't need to have scope access to
the given variables.

Fixed-Point Arithmetic
----------------------

With one exception, every piece of numeric logic in this game is done
using fixed-point (integer) arithmetic.  The game does so by internally
scaling up all pixel coordinates by a factor known as "Subpixel size".
These coordinates are scaled back down when drawing the game area
on the screen.  In practice, this means that every on-screen pixel
is further sub-divided into 65536 units (256 on each axis) by the
game's physics.  This prevents choppy movement without needing to rely
on floating-point numbers.

The reason I made this design choice is that floating-point numbers are
well-known to be inaccurate when performing additions or subtractions.
Many consecutive operations will gradually introduce small
inaccuracies, and these may slowly compound.  In all, floating-point
can have a tendency to be more difficult to reason about.  Fixed-point
arithmetic is used here to illustrate how it may be done.

Goto
----

The use of the goto keyword is rightly demonized by most programmers.
However, one aspect of programming in which goto is still one of
the best options is in error recovery and resource deallocation.
The function image_load() makes use of goto in cases where the file
to load cannot be read, or when memory needed to hold the file's
data cannot be allocated.  In these error cases, a goto is used that
jumps near the end of the function, to a label placed just before a
de-allocation.  This allows all manual resource management to happen
in one place, with deallocations being skipped if they logically
could not have happened.

Other languages solve the problem of deallocation either by using
deconstructors (such as C++, which are slow and may have side-effects),
or garbage collection, such as Java or Go.  Both systems involve lots
of magic, where use of goto involves explicit deallocation following
rules established in the code itself.

Speaking of Go, it too has a facility used similarly to the recommended
usage of goto, itself called "defer".  In Go's defer, a function
call is "deferred" to just before the current function returns.
In principle, this functions almost identically to goto-based
deallocations, except the language doesn't need to implement the
goto keyword at all.  There are currently proposals to include defer
in a future standard of the C language, so it is possible that in
the future, goto will become a less ideal solution for this type of
resource management.

Further Reading
===============

For general information on programming in C, I would recommend reading
the The C Programming Language, 2nd Edition by Brian W. Kernigan and
Dennis Ritchie.  Dennis was the creator of C, and Brian Kernigan is
an accomplished professor in Computer Science.  Both were instrumental
in the creation of the UNIX operating system at Bell Labs.

If you have any specific questions about the source code, you can
e-mail me at victoria.a.lacroix@gmail.com.  I'm receptive to unprompted
messages, so don't be shy.

Copyright
=========

pong.c (c) 2020 Victoria Lacroix

pong.c is distributed under the terms of an X11 license.  Please read
COPYING.txt for more information.
