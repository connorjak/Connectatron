set(CMAKE_CXX_STANDARD 17)

add_example(Connectatron
    Connectatron.cpp
)

find_package(blueprint-utilities REQUIRED)

target_link_libraries(Connectatron PRIVATE blueprint-utilities)

find_package(magic_enum CONFIG REQUIRED)
target_link_libraries(Connectatron PRIVATE magic_enum::magic_enum)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(Connectatron PRIVATE nlohmann_json nlohmann_json::nlohmann_json)