# Malloc

This is a custom implementation of malloc, using mmap(2) and munmap(2) to request and return memory pages.

It provides an efficient allocation process, designed to reduce fragmentation as much as possible while maintaining fast performances.

## Getting started

You must have [cmake](https://cmake.org/) installed on your system to build the application.

Clone the repository and build the library using `make`.
You will end up with two files:
- The proper library `libft_malloc_x86_64.so`
- A symbolic link to the library `libft_malloc.so`. Use this one to include in your own applications.

For the compiler to know where to look for the library, use `-rpath=[path]` and replace the `[path]` with the directory path containing the `libft_malloc.so`. Don't forget to also add `-lft_malloc`.
