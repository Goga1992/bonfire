bon_add_library(bon_logger)

find_package(spdlog REQUIRED)

target_link_libraries(bon_logger spdlog::spdlog)