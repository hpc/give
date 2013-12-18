========================================================================
                      Managed String Library
========================================================================

Description:

This managed string library was developed in response to the need for a string library that could improve the quality and security of newly developed C language programs while eliminating obstacles to widespread adoption and possible standardization.

The managed string library is based on a dynamic approach in that memory is allocated and reallocated as required. This approach eliminates the possibility of unbounded copies, null-termination errors, and truncation by ensuring there is always adequate space available for the resulting string (including the terminating null character).

A runtime-constraint violation occurs when memory cannot be allocated. In this way, the managed string library accomplishes the goal of succeeding or failing loudly.
The managed string library also provides a mechanism for dealing with data sanitization by (optionally) checking that all characters in a string belong to a predefined set of .safe. char-acters.

/////////////////////////////////////////////////////////////////////////////

Build instructions:

Microsoft Visual Studio
	  Import the project file and rebuild your solution. Everything will be taken care of auto-magically.

Mac OS X
	Copy Makefile.osx to Makefile.in, and run `make`.

gcc
	Copy Makefile.gcc to Makefile.in, and run `make`. (this is the default)

/////////////////////////////////////////////////////////////////////////////

Known issues:

The fprintf_m() and sprintf_m() functions currently lack functionality to print out arguments using the double or float specifiers.

/////////////////////////////////////////////////////////////////////////////

Compiled and tested under:

Windows XP using Visual C++ 2005 Express Edition
Windows XP Professional Version 2002 Service Pack 2 using Visual Studio 2005 Version 8.0.50727.42  
RedHat  gcc 3.4.4
Ubuntu  gcc 4.2.4 and 4.3.2
Mac OSX gcc 4.0.1 and 4.2.1

