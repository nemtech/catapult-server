NOTE: Commands are using `\` as marker for line continuations

Building on Ubuntu 18.04 (LTS)
===

Prerequisites
---

 * cmake (at least 3.14)
 * git
 * python 3.x
 * gcc 9.2
 * ninja-build - suggested
 * curl - suggested

Instructions below are for gcc, but project compiles with clang 9 as well.

Boost
---

```sh
curl -o boost_1_71_0.tar.gz -SL \
    https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz
tar -xzf boost_1_71_0.tar.gz

## WARNING: below use $HOME rather than ~ - boost scripts might treat it literally
mkdir boost-build-1.71.0
cd boost_1_71_0
./bootstrap.sh --prefix=${HOME}/boost-build-1.71.0
./b2 --prefix=${HOME}/boost-build-1.71.0 --without-python -j 4 stage release
./b2 --prefix=${HOME}/boost-build-1.71.0 --without-python install
```

Gtest
---

```sh
git clone https://github.com/google/googletest.git googletest.git
cd googletest.git
git checkout release-1.8.1

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON ..
make
sudo make install
```

Google benchmark
---

```sh
git clone https://github.com/google/benchmark.git google.benchmark.git
cd google.benchmark.git
git checkout v1.5.0

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DBENCHMARK_ENABLE_GTEST_TESTS=OFF ..
make
sudo make install
```

Mongo
---

mongo-c

```sh
git clone https://github.com/mongodb/mongo-c-driver.git mongo-c-driver.git
cd mongo-c-driver.git
git checkout 1.15.1

mkdir _build && cd _build
cmake -DENABLE_AUTOMATIC_INIT_AND_CLEANUP=OFF -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

mongocxx
```sh
git clone https://github.com/nemtech/mongo-cxx-driver.git mongo-cxx-driver.git
cd mongo-cxx-driver.git
git checkout r3.4.0-nem

mkdir _build && cd _build
cmake -DCMAKE_CXX_STANDARD=17 -DLIBBSON_DIR=/usr/local -DLIBMONGOC_DIR=/usr/local \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

ZMQ
---

libzmq
```sh
git clone git://github.com/zeromq/libzmq.git libzmq.git
cd libzmq.git
git checkout v4.3.2

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

cppzmq
```sh
git clone https://github.com/zeromq/cppzmq.git cppzmq.git
cd cppzmq.git
git checkout v4.4.1

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DCPPZMQ_BUILD_TESTS=OFF ..
make
sudo make install
```

Rocks
---

Currently ubuntu 18.04 has gflags in version 2.2.1 and snappy in version 1.1.7 which are OK

rocks
```sh
git clone https://github.com/nemtech/rocksdb.git rocksdb.git
cd rocksdb.git
git checkout v6.2.4-nem

mkdir _build && cd _build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local -DWITH_TESTS=OFF ..
make
sudo make install
```

CATAPULT
---

```sh
git clone https://github.com/nemtech/catapult-server.git
cd catapult-server

mkdir _build && cd _build
cmake -DBOOST_ROOT=~/boost-build-1.71.0 -DCMAKE_BUILD_TYPE=Release -DGTEST_ROOT=/usr/local/ -G Ninja ..
ninja publish
ninja -j$(nproc)
```
