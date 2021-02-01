# Raygun as Shared Library

Raygun can be built as shared library with just a few lines added to the main `CMakeLists.txt`.

First, mark the Raygun library shared:

```cmake
add_library(raygun SHARED ${raygun_srcs})
```

Next, enable the `raygun_copy_dlls` at the bottom:

```cmake
raygun_copy_dlls($<TARGET_FILE:raygun>)
```

Finally, uncomment the `RAYGUN_DLL_API` macro handling in `raygun/utils/macros.hpp`.
