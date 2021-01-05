# Linux Build Instructions

1. Compile [this fork of PhysX](https://github.com/W4RH4WK/PhysX).

   It's essentially vanilla PhysX with runtime type information enabled and some minor fixes to make it compile on recent systems.
   Follow the official build instructions.
   Note that there are dedicated build folders for *release* and *debug*.
   You don't need to install PhysX, we'll use it directly from the build directory.

2. Install the [Vulkan-SDK](https://www.lunarg.com/vulkan-sdk/) in version `1.2.162`.

3. Run the following command to install the remaining dependencies.
   Adjust package manager and package names accordingly if you are not using Ubuntu.

       sudo apt install ninja-build pkg-config libassimp-dev libglfw3-dev libopenal-dev libogg-dev libopus-dev libopusfile-dev

4. Run the following commands from the root of this repository to build Raygun.
   Adjust the path to PhysX accordingly.

       cmake -GNinja -DCMAKE_BUILD_TYPE=Release -DPhysX_ROOT=$HOME/git/PhysX -B build
       ninja -C build

5. You can now run the example:

       build/example/example

   Note that Raygun expects the `resources` directory to be located in the current working directory.
