cmake_minimum_required(VERSION 3.21)

# Set your project name. This will be the name of your SKSE .dll file.
project(NNDDynamicReveal VERSION 1.0.0 LANGUAGES CXX)

# #
# YOU DO NOT NEED TO EDIT ANYTHING BELOW HERE
# #

# If you're not using a mod manager, you probably want the SKSE plugin to go
# inside of your Skyrim "Data" folder.
if(DEFINED ENV{SKYRIM_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_FOLDER}/Data")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_FOLDER}/Data")
endif()

# If you're using Mod Organizer 2 or Vortex, you might want this to go inside
# of your "mods" folder, inside of a subfolder named "<your mod>".
if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
    set(OUTPUT_FOLDER "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
endif()

# Setup your SKSE plugin as an SKSE plugin!
find_package(CommonLibSSE CONFIG REQUIRED)
add_commonlibsse_plugin(${PROJECT_NAME} SOURCES plugin.cpp)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_precompile_headers(${PROJECT_NAME} PRIVATE PCH.h)

# Build a MO2-ready package and zip after each successful build:
#   build/release/package/SKSE/Plugins/NNDDynamicReveal.dll
#   build/release/NNDDynamicReveal.zip  <- drag this into MO2
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
set(SKYRIMNET_PROMPT_DIR "${PACKAGE_DIR}/SKSE/Plugins/SkyrimNet/prompts/submodules/character_bio")
file(MAKE_DIRECTORY "${PACKAGE_DIR}/SKSE/Plugins")
file(MAKE_DIRECTORY "${SKYRIMNET_PROMPT_DIR}")
add_custom_command(
    TARGET "${PROJECT_NAME}" POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${PACKAGE_DIR}/SKSE/Plugins/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/NNDDynamicReveal.ini"
        "${PACKAGE_DIR}/SKSE/Plugins/NNDDynamicReveal.ini"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/SkyrimNet/prompts/submodules/character_bio/0005_nnd_name.prompt"
        "${SKYRIMNET_PROMPT_DIR}/0005_nnd_name.prompt"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_SOURCE_DIR}/vendor/NPCsNamesDistributor.dll"
        "${PACKAGE_DIR}/SKSE/Plugins/NPCsNamesDistributor.dll"
    COMMAND "${CMAKE_COMMAND}" -E tar cf
        "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.zip" --format=zip -- "SKSE"
    WORKING_DIRECTORY "${PACKAGE_DIR}"
    VERBATIM
)

# Optional: auto-deploy directly into Skyrim or a mod manager.
if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")
    message(STATUS "SKSE plugin output folder: ${DLL_FOLDER}")

    add_custom_command(
        TARGET "${PROJECT_NAME}" POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DLL_FOLDER}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_SOURCE_DIR}/NNDDynamicReveal.ini" "${DLL_FOLDER}/NNDDynamicReveal.ini"
        VERBATIM
    )

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
            TARGET "${PROJECT_NAME}" POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
            VERBATIM
        )
    endif()
endif()
