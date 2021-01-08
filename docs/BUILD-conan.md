# Building with Conan

Following instructions should work on Mac, Linux and Windows.

## Prerequisites

* [Conan](https://conan.io)

### Install and build Conan dependencies

While Conan will be building and installing packages, you might want to go for a ☕ (or lunch),
as this will probably take *a bit*.

```sh
conan remote add nemtech https://api.bintray.com/conan/nemtech/symbol-server-dependencies

git clone https://github.com/nemtech/catapult-server.git
cd catapult-server

mkdir _build && cd _build
conan install .. --build missing
```

## Build instructions

### Windows + Visual Studio

Generate project files for VS 2019:

```sh
cmake -G "Visual Studio 16 2019" -A x64 -DUSE_CONAN=ON -DPYTHON_EXECUTABLE:FILEPATH=X:/python3x/python.exe ..
```

Generate project files for VS 2017:

```sh
cmake -G "Visual Studio 15 2017 Win64" -DUSE_CONAN=ON -DPYTHON_EXECUTABLE:FILEPATH=X:/python3x/python.exe ..
```

Compilation:

```sh
cmake --build . --target publish
msbuild /p:Configuration=RelWithDebInfo /p:Platform=x64 ALL_BUILD.vcxproj
```

### Linux and macOS

```sh
cmake -DUSE_CONAN=ON -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja publish
ninja -j4
```
