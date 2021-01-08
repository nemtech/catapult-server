# Building from source

This guide should mostly work for Linux and OS X operating systems.
It does not include instructions for Windows.
For Windows, [Build with CONAN](BUILD-conan.md) is strongly encouraged.

## Prerequisites

Required:

- OpenSSL dev library (`libssl-dev`) >= 1.1.1g.
- `pkg-config` (for zeromq).
- `zsh` (for Step 2). This shell is known to work, other shells have not been tested.
- An environment variable named `CAT_DEPS_DIR` defined as the directory containing all catapult dependencies.
- About 15 GB of free disk space.

These instructions have been verified to work on Ubuntu 20.04 with 8 GB of RAM and 4 CPU cores.

### Installing the prerequisites on Ubuntu

This code snippet installs the aforementioned prerequisites on apt-based systems (like Ubuntu), creates a `cat_deps_dir` folder under `$HOME` and points `CAT_DEPS_DIR` to it.

Copy & paste it into a terminal (please mind the blank lines):

```sh
sudo apt update
sudo apt -y upgrade
sudo apt -y install git gcc g++ cmake curl libssl-dev ninja-build zsh pkg-config

zsh

mkdir -p cat_deps_dir
export CAT_DEPS_DIR=$HOME/cat_deps_dir
```

> **NOTE**:
> If you want the `CAT_DEPS_DIR` environment variable to persist across sessions make sure to include the last line in the `~/.profile` or `~/.bashrc` files.

## Step 1: Download all dependencies from source

Copy & paste the whole snippet below into a terminal:

```sh
function download_boost {
	local boost_ver=1_${1}_0
	local boost_ver_dotted=1.${1}.0

	curl -o boost_${boost_ver}.tar.gz -SL https://dl.bintray.com/boostorg/release/${boost_ver_dotted}/source/boost_${boost_ver}.tar.gz
	tar -xzf boost_${boost_ver}.tar.gz
	mv boost_${boost_ver} boost
}

function download_git_dependency {
	git clone git://github.com/${1}/${2}.git
	cd ${2}
	git checkout ${3}
	cd ..
}

function download_all {
	download_boost 75

	download_git_dependency google googletest release-1.10.0
	download_git_dependency google benchmark v1.5.2

	download_git_dependency mongodb mongo-c-driver 1.17.2
	download_git_dependency mongodb mongo-cxx-driver r3.6.1

	download_git_dependency zeromq libzmq v4.3.3
	download_git_dependency zeromq cppzmq v4.7.1

	download_git_dependency facebook rocksdb v6.13.3
}

cd ${CAT_DEPS_DIR}
mkdir source
cd source
download_all
```

## Step 2: Build and install all dependencies

Copy & paste the whole snippet below into a terminal:

```sh
boost_output_dir=${CAT_DEPS_DIR}/boost

function install_boost {
	cd boost
	./bootstrap.sh with-toolset=clang --prefix=${boost_output_dir}

	b2_options=()
	b2_options+=(--prefix=${boost_output_dir})
	./b2 ${b2_options[@]} -j 8 stage release
	./b2 install ${b2_options[@]}
}

function install_git_dependency {
	cd ${2}
	mkdir _build
	cd _build

	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX="${CAT_DEPS_DIR}/${1}" ${cmake_options[@]} ..
	make -j 8 && make install
}

function install_google_test {
	cmake_options=()
	cmake_options+=(-DCMAKE_POSITION_INDEPENDENT_CODE=ON)
	install_git_dependency google googletest
}

function install_google_benchmark {
	cmake_options=()
	cmake_options+=(-DBENCHMARK_ENABLE_GTEST_TESTS=OFF)
	install_git_dependency google benchmark
}

function install_mongo_c_driver {
	cmake_options=()
	cmake_options+=(-DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF)
	cmake_options+=(-DENABLE_MONGODB_AWS_AUTH=OFF)
	cmake_options+=(-DENABLE_TESTS=OFF)
	cmake_options+=(-DENABLE_EXAMPLES=OFF)
	cmake_options+=(-DENABLE_SASL=OFF)
	install_git_dependency mongodb mongo-c-driver
}

function install_mongo_cxx_driver {
	cmake_options=()
	cmake_options+=(-DBOOST_ROOT=${boost_output_dir})
	cmake_options+=(-DCMAKE_CXX_STANDARD=17)
	install_git_dependency mongodb mongo-cxx-driver
}

function install_zmq_lib {
	cmake_options=()
	cmake_options+=(-DWITH_TLS=OFF)
	install_git_dependency zeromq libzmq
}

function install_zmq_cpp {
	cmake_options=()
	cmake_options+=(-DCPPZMQ_BUILD_TESTS=OFF)
	install_git_dependency zeromq cppzmq
}

function install_rocks {
	cmake_options=()
	cmake_options+=(-DPORTABLE=1)
	cmake_options+=(-DWITH_TESTS=OFF)
	cmake_options+=(-DWITH_TOOLS=OFF)
	cmake_options+=(-DWITH_BENCHMARK_TOOLS=OFF)
	cmake_options+=(-DWITH_CORE_TOOLS=OFF)
	cmake_options+=(-DWITH_GFLAGS=OFF)
	install_git_dependency facebook rocksdb
}

function install_all {
	declare -a installers=(
		install_boost
		install_google_test
		install_google_benchmark
		install_mongo_c_driver
		install_mongo_cxx_driver
		install_zmq_lib
		install_zmq_cpp
		install_rocks
	)
	for install in "${installers[@]}"
	do
		pushd source > /dev/null
		${install}
		popd > /dev/null
	done
}

cd ${CAT_DEPS_DIR}
install_all
```

## Step 3: Download and build catapult

Finally, copy & paste the whole snippet below into a terminal:

```sh
git clone https://github.com/nemtech/catapult-server.git
cd catapult-server

mkdir _build && cd _build
BOOST_ROOT="${CAT_DEPS_DIR}/boost" cmake .. \
	-DCMAKE_BUILD_TYPE=RelWithDebInfo \
	-DCMAKE_PREFIX_PATH="${CAT_DEPS_DIR}/facebook;${CAT_DEPS_DIR}/google;${CAT_DEPS_DIR}/mongodb;${CAT_DEPS_DIR}/zeromq" \
	\
	-GNinja
ninja publish
ninja -j4
```

> **NOTE:**
> On macOS, use ':' as separator character instead of ';' for ``CMAKE_PREFIX_PATH``.

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
