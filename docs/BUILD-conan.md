# Building with conan

Following instructions should work both on mac, linux, windows

NOTE: Commands are using `\` as marker for line continuations

## Prerequisites

 * [conan](https://conan.io)

### Install and build conan dependencies

While conan will be building and installing packages, you might want to go for a ☕ (or lunch),
as this will probably take *a bit*.

```sh
conan remote add nemtech https://api.bintray.com/conan/nemtech/symbol-server-dependencies

git clone https://github.com/nemtech/catapult-server.git
cd catapult-server

mkdir _build && cd _build
conan install .. --build missing
```

OS dependent instructions:

**Windows + MSVC**

```sh
cmake -G "Visual Studio 15 2017 Win64" -DUSE_CONAN=ON -DPYTHON_EXECUTABLE:FILEPATH=X:/python3x/python.exe ..
cmake --build . --target publish
msbuild /p:Configuration=RelWithDebInfo /p:Platform=x64 ALL_BUILD.vcxproj
```

**Linux + MacOS**
```sh
cmake -DUSE_CONAN=ON -DCMAKE_BUILD_TYPE=Release -G Ninja ..
ninja publish
ninja -j4
```
