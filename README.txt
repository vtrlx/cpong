pong.c
======

A clone of Pong written in suckless-style C.

1 Installation
--------------

1.1 Windows
-----------

To compile this on a Windows system, visit the MSYS2 website
(https://www.msys2.org/) and follow the instructions in the "Installation"
section of the main page.

Once it is installed, open the MSYS2 MinGW64 terminal then run the
following commands (without the '$'):

	$ pacman -S mingw32-make
	$ pacman -S libglfw3
	$ pacman -S libpng
	$ pacman -S zlib

1.2 Linux / *BSD / etc
----------------------

Using your operating system's package manager, install the following
packages:

	- gcc
	- make
	- libglfw3
	- libpng
	- zlib

The packages `gcc` and `make` are likely to already be installed on
your system.  If not, they will usually be brought in by a meta-package
named like `build-essential` or `base-devel`.

In most distributions of Linux, you will need to install the development
versions of the packages `libglfw3`, `libpng`, and `zlib`.  Most of the
time, these packages will be suffixed with `-dev` or `-devel`.

2 Compiling
-----------

Once the development environment and libraries are installed, the
program can be compiled by invoking the command `make` in this directory.
Windows users will want to run `make` using the MinGW64 terminal instead
of the MSYS2 terminal.  Linux/*BSD users may also install the program
to their PATH by running the command `make install` as root.

	# make install

By default, the program is installed to `/usr/local/bin`.  To install
in a different location, set the value of the PREFIX variable when
running `make install` e.g.:

	# PREFIX=$HOME/bin make install

3 Running
---------

Run the game with the `pong` command.  The human player controls their
paddle (on the left) using the arrow keys `up` and `down`.  To quit the
game, press the `Escape` key.

4 More Information
------------------

For general information on programming in C, I would recommend reading
the The C Programming Language, 2nd Edition by Brian W. Kernigan and
Dennis Ritchie.  Dennis was the creator of C, and Brian Kernigan is an
accomplished professor in Computer Science.  Both were instrumental in
the creation of the UNIX operating system at Bell Labs.

For more information on how to write suckless-style C, take a look at
suckless.org's style guide (https://suckless.org/coding_style/).
suckless.org hosts and maintains a small number of extremely simple
utilities to be used on UNIX-like systems, notably the window manager
`dwm` and the web browser `surf`.

If you have any specific questions about the source code, you can e-mail
me at victoria.a.lacroix@gmail.com.  I'm receptive to unprompted
messages, so don't be shy.

5 Copyright
-----------

pong.c (c) 2020 Victoria Lacroix

pong.c is distributed under the terms of an MIT/X Consortium license.
Please read COPYING for more information.
