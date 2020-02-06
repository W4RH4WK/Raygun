# Windows Build Instructions

1. Install [CMake](https://cmake.org/).

   Add CMake to the path of the current user.

2. Install the [Vulkan-SDK](https://www.lunarg.com/vulkan-sdk/).

3. Generate the Visual Studio Solution:

   Open *Developer Command Prompt for VS 2019* and navigate to the repository.
   Run the following command:

       cmake.exe -G "Visual Studio 16 2019" -A x64 -B build

4. Open the generated solution `raygun.sln` located inside the `build` directory.

5. Release and debug configurations can now be built and run from within Visual Studio.
