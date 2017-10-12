Sunnet
======
<img src="sunnet.png" width="200" height="200" />

Sunnet is a platform agnostic, modern C++ wrapper around BSD Sockets/WinSock 
to be used for network communication.

This library was originally developed for a real-time space strategy game
and has since been moved into its own project.

## Building and Using
SunNet uses CMake for its build process. 

## Technical Information
This is meant to be a super-simple drop in for network communication using
a channeled-callback strategy. SunNet makes the following assumptions:

1. All computers communicating on the network have the same byte ordering
2. All computers communicating on the network serialize data the same way (ie same compiler, architecture)


A few goals remain:


0. Ensure actual Linux compat
1. Port to CMake for true cross-platform support
2. Enforce byte-ordering to make platform agnostic
3. Home grown serialization to make platform agnostic
4. Overall code improvement
