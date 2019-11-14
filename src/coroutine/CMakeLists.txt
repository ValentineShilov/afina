# build service
set(SOURCE_FILES
    Engine.cpp
)
set(SANITIZER_FLAGS "  -fno-sanitize=all -fno-sanitize-recover=all")
add_library(Coroutine ${SOURCE_FILES})
