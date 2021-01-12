# Building with Conan

Following instructions should work on Mac, Linux and Windows.

## Prerequisites

* Install [Conan](https://conan.io)

* **On Linux**, if this is the first time running Conan, you need to set the right C++ ABI:

  ```sh
  conan profile new default --detect
  conan profile update settings.compiler.libcxx=libstdc++11 default
  ```

* **On Windows**, run all the commands below from a command prompt that has access to Visual Studio and Git. This can be accomplished by using the "Native Tools Command Prompt" shortcut installed by Visual Studio on the Start Menu.

## Step 1: Build dependencies

While Conan will be building and installing packages, you might want to go for a ☕ (or lunch),
as this will probably take *a bit*.

```sh
conan remote add nemtech https://api.bintray.com/conan/nemtech/symbol-server-dependencies

git clone https://github.com/nemtech/catapult-server.git
cd catapult-server

mkdir _build && cd _build
conan install .. --build missing
```

## Step 2: Build catapult

### Windows + Visual Studio

> **NOTE:**
> Make sure to use the correct ``PYTHON_EXECUTABLE`` path! Python3 is required for the build to produce some header files. If Python3 cannot be found you won't notice until more than one hour into the build process because of some missing headers.

* Generate project files for VS 2019:

  ```sh
  cmake -G "Visual Studio 16 2019" -A x64 -DUSE_CONAN=ON -DPYTHON_EXECUTABLE:FILEPATH=X:/python3x/python.exe ..
  ```

* Generate project files for VS 2017:

  ```sh
  cmake -G "Visual Studio 15 2017 Win64" -DUSE_CONAN=ON -DPYTHON_EXECUTABLE:FILEPATH=X:/python3x/python.exe ..
  ```

* Build:

  ```sh
  cmake --build . --target publish
  msbuild /p:Configuration=RelWithDebInfo /p:Platform=x64 ALL_BUILD.vcxproj
  ```

  After building successfully, the tools in ``_build\bin`` are ready to use. All runtime dependencies have been copied into the same folder so Windows will find them.

* Verify:

  Check that the tools are working correctly by running:

  ```sh
  bin\catapult.tools.address --help
  ```

### Linux and macOS

* Build:

  ```sh
  cmake -DUSE_CONAN=ON -DCMAKE_BUILD_TYPE=Release -G Ninja ..
  ninja publish
  ninja -j4
  ```

  Once the build finishes successfully, the tools in ``_build/bin`` are ready to use. However, the dependencies in ``_build/deps`` must be accessible so make sure to add this folder to the ``LD_LIBRARY_PATH`` environment variable (Linux) or ``DYLD_LIBRARY_PATH`` (Mac).

* Install (Optional):

  The catapult tools can be made available globally by running:

  ```sh
  sudo ninja install
  ```

  > **NOTE:**
  > You can change the default installation location by passing ``-DCMAKE_INSTALL_PREFIX=...`` to ``cmake`` in the Build step. In this case you might not require ``sudo``.

* Verify:

  Check that the tools are working correctly by running:

  ```sh
  bin/catapult.tools.address --help
  ```
