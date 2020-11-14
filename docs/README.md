# Developer Notes

This document is intended for developers interested in catapult-server development.

## Hardware requirements

The scripts shared on this page have been tested on servers with the following minimum requirements.

* CPU: 2 cores or more
* Memory: 4GB or more
* HD: 20GB or more

Server requirements are network dependent.
For example, networks with higher throughput will likely have higher requirements.

Although you might be able to run the software in less powerful instances, you might encounter some issues while installing or running the node.

## Port requirements

The port ``7900`` is required by catapult-server to communicate between nodes.
Make sure that the server's host is accessible from the internet and that the port is open and available.

## Building catapult-server

### Prerequisites

 * cmake >= 3.14
 * python 3.x

### Compilers

Catapult's supported compilers are:

Linux:
 - Clang: 11.0.1
 - Clang: 10.0.0
 - GCC: 10.2.0
 - GCC: 9.2.1

OS X:
 - Apple Clang: 11.0.3

Windows:
 - Visual C++: 15.8

### Guides

 * [Build with CONAN](BUILD-conan.md)
 * [Build without CONAN](BUILD-manual.md)

### Sanitizers

There are a few false positives when running sanitizers on targets
compiled with clang 9.
When building sanitizers, `sanitizer_blacklist.txt` file is used.

When running thread sanitizer, there are following suppressions required:

 * for false positive in libc++ `shared_ptr`:
 * for false positive in boost logger, in server logger is always initialized from a single thread

```
race:~weak_ptr
race:global_logger::get()
```

## Running a private network

* [Linux instructions](RUNNETWORKLIN.md)
* [Network configuration](https://nemtech.github.io/guides/network/configuring-network-properties.html)

## Running a peer node

* [Linux instructions](RUNPEERLIN.md)
* [Node configuration](https://nemtech.github.io/guides/network/configuring-node-properties.html)
