cmake_minimum_required(VERSION 3.7)

project(e2fsimage C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_definitions(-DVER="0.3.0")

add_executable(e2fsimage
        src/copy.c
        src/dirent.c
        src/e2fsimage.h
        src/group.c
        src/inodb.c
        src/main.c
        src/malloc.c
        src/mkdir.c
        src/mke2fs.c
        src/passwd.c
        src/sfile.c
        src/symlink.c
        src/uiddb.c
        src/uids.c
        src/util.c
        src/xattr.c)
target_link_libraries(e2fsimage ext2fs com_err)
