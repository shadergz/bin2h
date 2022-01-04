cmake_minimum_required (VERSION 3.14)

project (bin2h
    VERSION 0.0.1

)

set (bin2h_SRC
    "bin2h.c"
)

if (!UNIX)
list (APPEND bin2h_SRC
    "getopt.c"
)
endif ()

add_executable (${PROJECT_NAME} ${bin2h_SRC})

