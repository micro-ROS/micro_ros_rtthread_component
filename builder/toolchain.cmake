include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_CROSSCOMPILING 1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

set(CMAKE_C_COMPILER tricore-gcc)
set(CMAKE_CXX_COMPILER tricore-g++)
set(CMAKE_AR tricore-ar)

set(CMAKE_C_FLAGS_INIT " -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g  -DCLOCK_MONOTONIC=0 -DCLOCK_REALTIME=1  -D'__attribute__(x)='" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_INIT " -ffunction-sections -fdata-sections -Dgcc -O0 -gdwarf-2 -g  -fno-rtti -DCLOCK_MONOTONIC=0 -DCLOCK_REALTIME=1 -D'__attribute__(x)='" CACHE STRING "" FORCE)

set(__BIG_ENDIAN__ 0)