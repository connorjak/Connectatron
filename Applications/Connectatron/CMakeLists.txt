add_example_executable(Connectatron
    Connectatron.cpp
    utilities/builders.h
    utilities/drawing.h
    utilities/widgets.h
    utilities/builders.cpp
    utilities/drawing.cpp
    utilities/widgets.cpp
)

find_package(magic_enum CONFIG REQUIRED)
target_link_libraries(Connectatron PRIVATE magic_enum::magic_enum)

find_package(nlohmann_json CONFIG REQUIRED)
target_link_libraries(Connectatron PRIVATE nlohmann_json nlohmann_json::nlohmann_json)