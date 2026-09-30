cmake_minimum_required(VERSION 3.16)
project(BooksSongs VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W3 /utf-8)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

# Platform libraries
if(WIN32)
    set(PLATFORM_LIBS ws2_32)
else()
    find_package(Threads REQUIRED)
    set(PLATFORM_LIBS Threads::Threads)
endif()

add_executable(books-songs-server
    backend/src/main.cpp
    backend/src/handlers.cpp
    backend/src/mailer.cpp
)

target_include_directories(books-songs-server PRIVATE backend/src)
target_link_libraries(books-songs-server PRIVATE ${PLATFORM_LIBS})

# ── Post-build: copy frontend next to the executable ──────────────────────────
add_custom_command(TARGET books-songs-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/frontend"
        "$<TARGET_FILE_DIR:books-songs-server>/frontend"
    COMMENT "Copying frontend"
)

# ── Post-build: create data directories ───────────────────────────────────────
add_custom_command(TARGET books-songs-server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:books-songs-server>/data"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:books-songs-server>/data/audio"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:books-songs-server>/data/avatars"
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:books-songs-server>/data/covers"
    COMMENT "Creating data directories"
)
