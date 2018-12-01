Catapult is x64 only, there are no 32-bit builds.

Dependencies:

 * cmake
 * python 3 - various scripts
 * boost
 * gtest
 * mongo + mongo-cxx
 * zmq
 * rocksdb

Variables required to build catapult:

 * `PYTHON_EXECUTABLE`
 * `BOOST_ROOT`
 * `GTEST_ROOT`
 * `LIBBSONCXX_DIR`
 * `LIBMONGOCXX_DIR`
 * `ZeroMQ_DIR`
 * `cppzmq_DIR`
 * win: `RocksDB_DIR`, \*nix: `ROCKSDB_ROOT_DIR`

Once you have all variables set up correctly, build becomes trivial.

Prepare build directory:
```
mkdir _build
cd _build
```

Generate makefiles and build:
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo ..
make publish
make -j 4
```

Or use generator of your choice
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo -G Ninja ..
ninja publish
ninja
```

VS:
```
cmake -DCMAKE_BUILD_TYPE=RelWithDebugInfo -G "Visual Studio 14 2015 Win64" ..
# Open up catapult_server.sln
```

