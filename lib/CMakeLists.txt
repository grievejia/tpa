include_directories (${tpa_SOURCE_DIR}/include)

add_subdirectory(Client)
add_subdirectory(MemoryModel)
add_subdirectory(PointerAnalysis)
add_subdirectory(TPA)
add_subdirectory(Transforms)
#add_subdirectory(gtest-minimal)