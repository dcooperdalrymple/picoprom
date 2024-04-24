add_library(littlefs)

target_sources(littlefs PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs.c
    ${CMAKE_CURRENT_LIST_DIR}/littlefs/lfs_util.c
)

target_include_directories(littlefs PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/littlefs
)
