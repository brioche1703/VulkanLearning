file(GLOB BASE_SRC "*.cpp" "*.hpp" "*.h" "../external/imgui/*.cpp")
file(GLOB BASE_HEADERS "*.hpp" "*.h")

set(OTHER_SOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
file(GLOB OTHER_SOURCES 
    ${OTHER_SOURCES_DIR}/misc/*.cpp
    ${OTHER_SOURCES_DIR}/misc/model/*.cpp
    ${OTHER_SOURCES_DIR}/window/*.cpp
    ${OTHER_SOURCES_DIR}/*.cpp
    ${OTHER_SOURCES_DIR}/camera/*.cpp
    )

set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../external/KTX-Software)
set(KTX_SOURCES
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/texture1.c
    ${KTX_DIR}/lib/texture2.c
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/dfdutils/createdfd.c
    ${KTX_DIR}/lib/dfdutils/colourspaces.c
    ${KTX_DIR}/lib/dfdutils/dfd.h
    ${KTX_DIR}/lib/dfdutils/dfd2vk.inl
    ${KTX_DIR}/lib/dfdutils/interpretdfd.c
    ${KTX_DIR}/lib/dfdutils/printdfd.c
    ${KTX_DIR}/lib/dfdutils/queries.c
    ${KTX_DIR}/lib/dfdutils/vk2dfd.c
    ${KTX_DIR}/lib/dfdutils/vulkan/vk_platform.h
    ${KTX_DIR}/lib/dfdutils/vulkan/vulkan_core.h
    ${KTX_DIR}/other_include/zstd.h
    ${KTX_DIR}/lib/zstddeclib.c
    )

add_library(base STATIC ${BASE_SRC} ${OTHER_SOURCES} ${KTX_SOURCES})
target_link_libraries(base glfw)
if(WIN32)
    target_link_libraries(base ${Vulkan_LIBRARY} ${WINLIBS})
 else(WIN32)
    target_link_libraries(base ${Vulkan_LIBRARY} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif(WIN32)
