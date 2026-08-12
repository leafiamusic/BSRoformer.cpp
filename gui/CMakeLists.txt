# Qt6 GUI for BSRoformer.cpp
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)

add_executable(bs_roformer-gui
    main.cpp
    mainwindow.cpp
    worker.cpp
)

target_link_libraries(bs_roformer-gui PRIVATE
    bs_roformer
    Qt6::Widgets
)

target_include_directories(bs_roformer-gui PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party
)

# Copy GGML runtime DLLs on Windows (matches CLI handling).
bsr_copy_ggml_runtime_dlls(bs_roformer-gui)
