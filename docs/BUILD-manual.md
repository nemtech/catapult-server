# Building from source

This guide should mostly work for Linux and OS X operating systems.
It does not include instructions for Windows.
For Windows, [Build with CONAN](BUILD-conan.md) is strongly encouraged.

## Prerequisites

Required:

- OpenSSL dev library (`libssl-dev`) >= 1.1.1g.
- `pkg-config` (for zeromq).
- An environment variable named `CAT_DEPS_DIR` defined as the directory containing all catapult dependencies.
- About 15 GB of free disk space.

These instructions have been verified to work on Ubuntu 20.04 with 8 GB of RAM and 4 CPU cores.

### Installing the prerequisites on Debian/Ubuntu

This program installs the aforementioned system-wide prerequisites on apt-based systems (like Ubuntu or Debian), creates a directory at the location pointed by the environment variable `CAT_DEPS_DIR`.

## Step 1: Clone catapult-server, prepare the system and environment

Copy & paste the whole snippet below into a terminal:
```sh
git clone https://github.com/nemtech/catapult-server.git
cd catapult-server
sudo scripts/configure-manual.sh install system_reqs
export CAT_DEPS_DIR=$HOME/cat_deps_dir
```

> **NOTE**:
> If you want the `CAT_DEPS_DIR` environment variable to persist across sessions make sure to include the last line in the `~/.profile` or `~/.bashrc` files.

## Optional step: Download, build and install all dependencies from source

Type this into a terminal:

```sh
scripts/configure-manual.sh install deps
```

> **NOTE**:
> If you only want to download the dependencies (without building and installing them) enter the command ``scripts/configure-manual.sh download deps`` instead. Type ``scripts/configure-manual.sh --help`` for its summary of commands.

## Step 2: Prepare build directory

For creating/updating the `_build` directory type into a terminal:

```sh
scripts/configure-manual.sh
```

It will handle if you missed the previous optional step.

## Step 3: build/rebuild catapult

Finally, to start compiling type into a terminal:

```sh
cd _build
ninja -j8
```

## Step 4: Installation

Once the build finishes successfully, the tools in ``_build/bin`` are ready to use. Optionally, they can be made available globally by running:

```sh
sudo ninja install
```

> **NOTE:**
> You can change the default installation location passing ``-DCMAKE_INSTALL_PREFIX=...`` to ``cmake``. In this case you might not require ``sudo``.

Regardless of whether the tools are installed globally or not, their dependencies must be accessible before running them so make sure to update ``LD_LIBRARY_PATH``:

```sh
export LD_LIBRARY_PATH=$CAT_DEPS_DIR/boost/lib:$CAT_DEPS_DIR/facebook/lib:$CAT_DEPS_DIR/google/lib:$CAT_DEPS_DIR/mongodb/lib:$CAT_DEPS_DIR/zeromq/lib
```

Verify that the tools are working correctly by running:

```sh
bin/catapult.tools.address --help
```
