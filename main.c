#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define MAX_PATH_LEN 1024
#define MAX_PROJECT_NAME 256

// clang-format off
#ifdef _WIN32
    #include <Windows.h>
    #define get_cwd(buf, size) GetCurrentDirectoryA(size, buf)
    #define PATH_SEP '\\'

    int file_exists(const char* path) {
        DWORD attr = GetFileAttributesA(path);
        return (attr != INVALID_FILE_ATTRIBUTES &&
            !(attr & FILE_ATTRIBUTE_DIRECTORY));
    }
#else
    #include <limits.h>
    #include <unistd.h>
    #define get_cwd(buf, size) getcwd(buf, size)
    #define PATH_SEP '/'

    int file_exists(const char* path) { 
        return access(path, F_OK) == 0;
    }
#endif
// clang-format on

bool create_cmakelists_file(const char* project_name);
bool create_cmakepresets_file(void);
void print_help(const char* program_name);

int main(int argc, const char* argv[]) {
    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n\n", argv[1]);
            print_help(argv[0]);
            return 1;
        }
    }
    char cwd[MAX_PATH_LEN];
    char project_name_buf[MAX_PROJECT_NAME];

    if (!get_cwd(cwd, sizeof(cwd))) {
        fprintf(stderr, "Error: Failed to get current working directory\n");
        return 1;
    }

    // Extract and sanitize project name
    const char* temp = strrchr(cwd, PATH_SEP);
    const char* project_name_src = temp ? temp + 1 : cwd;
    strncpy(project_name_buf, project_name_src, sizeof(project_name_buf) - 1);
    project_name_buf[sizeof(project_name_buf) - 1] = '\0';

    // Replace spaces with underscores
    for (char* p = project_name_buf; *p; ++p) {
        if (*p == ' ') {
            *p = '_';
        }
    }
    const char* project_name = project_name_buf;

    if (file_exists("CMakeLists.txt")) {
        fprintf(stderr,
                "Error: CMakeLists.txt already exists in the current "
                "directory\n");
        return 1;
    }

    if (create_cmakelists_file(project_name)) {
        return 1;
    }

    if (file_exists("CMakePresets.json")) {
        fprintf(stderr,
                "Error: CMakePresets.json already exists in the current "
                "directory\n");
        return 1;
    }

    if (create_cmakepresets_file()) {
        return 1;
    }

    return 0;
}

void print_help(const char* program_name) {
    printf("Usage: %s [OPTION]\n", program_name);
    printf(
        "Generate CMakeLists.txt and CMakePresets.json for a C/C++ "
        "project\n\n");
    printf("  -h, --help    Display this help message\n");
}

bool create_cmakelists_file(const char* project_name) {
    FILE* file = fopen("CMakeLists.txt", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to create CMakeLists.txt\n");
        return true;
    }

    fprintf(file, "cmake_minimum_required(VERSION 3.25)\n\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# Project\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "project(%s\n", project_name);
    fprintf(file, "\tLANGUAGES C CXX\n)\n");
    fprintf(file, "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# C Standard\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "set(CMAKE_C_STANDARD 23)\n");
    fprintf(file, "set(CMAKE_C_STANDARD_REQUIRED ON)\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# C++ Standard\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "set(CMAKE_CXX_STANDARD 23)\n");
    fprintf(file, "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# Executable\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "set(source_dir \"${PROJECT_SOURCE_DIR}/src\")\n");
    fprintf(file, "file(GLOB_RECURSE source_files\n");
    fprintf(file, "\tCONFIGURE_DEPENDS\n");
    fprintf(file, "\t\"${source_dir}/*.c\"\n");
    fprintf(file, "\t\"${source_dir}/*.cpp\"\n");
    fprintf(file, "\t\"${source_dir}/*.cc\"\n");
    fprintf(file, "\t\"${source_dir}/*.cxx\"\n)\n\n");
    fprintf(file, "add_executable(%s ${source_files})\n\n", project_name);

    fprintf(file, "target_include_directories(%s PRIVATE\n", project_name);
    fprintf(file, "\t\"${PROJECT_SOURCE_DIR}/include\"\n");
    fprintf(file, "\t\"${PROJECT_SOURCE_DIR}/external/include\"\n)\n\n");

    fprintf(file, "target_link_directories(%s PRIVATE\n", project_name);
    fprintf(file, "\t\"${PROJECT_SOURCE_DIR}/external/lib\"\n)\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# Compiler warnings\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "if(MSVC)\n");
    fprintf(file, "\ttarget_compile_options(%s PRIVATE /W4)\n\n", project_name);
    fprintf(file, "\tif (CMAKE_CXX_COMPILER)\n");
    fprintf(file, "\t\ttarget_compile_options(%s PRIVATE\n", project_name);
    fprintf(file,
            "\t\t\t/permissive- \n\t\t\t/Zc:__cplusplus\n\t\t)\n\tendif()\n");
    fprintf(file, "else()\n");
    fprintf(file, "\ttarget_compile_options(%s PRIVATE\n", project_name);
    fprintf(file, "\t\t-Wall\n\t\t-Wextra\n\t\t-Wpedantic\n\t)\n\n");
    fprintf(file, "\tif (CMAKE_CXX_COMPILER)\n");
    fprintf(file, "\t\ttarget_compile_options(%s PRIVATE -Wshadow)\n",
            project_name);
    fprintf(file, "\tendif()\n");
    fprintf(file, "endif()\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# Install\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "install(TARGETS %s DESTINATION bin)\n", project_name);
    fprintf(file, "install(DIRECTORY include/ DESTINATION include)\n");

    fclose(file);
    printf("Created: CMakeLists.txt\n");
    return false;
}

bool create_cmakepresets_file(void) {
    FILE* file = fopen("CMakePresets.json", "w");
    if (!file) {
        fprintf(stderr, "Error: Failed to create CMakePresets.json\n");
        return true;
    }

    fprintf(file,
            "{\n"
            "\t\"version\": 6,\n"
            "\t\"cmakeMinimumRequired\": {\n"
            "\t\t\"major\": 3,\n"
            "\t\t\"minor\": 25,\n"
            "\t\t\"patch\": 0\n"
            "\t},\n\n"

            "\t\"configurePresets\": [\n"
            "\t\t{\n"
            "\t\t\t\"name\": \"base\",\n"
            "\t\t\t\"hidden\": true,\n"
            "\t\t\t\"generator\": \"Ninja\",\n"
            "\t\t\t\"binaryDir\": \"${sourceDir}/build/${presetName}\",\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_EXPORT_COMPILE_COMMANDS\": true\n"
            "\t\t\t}\n"
            "\t\t},\n\n"

            "\t\t{\n"
            "\t\t\t\"name\": \"win-gcc-debug\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Windows\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"gcc\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"g++\"\n"
            "\t\t\t}\n"
            "\t\t},\n"
            "\t\t{\n"
            "\t\t\t\"name\": \"win-gcc-release\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Windows\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"gcc\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"g++\"\n"
            "\t\t\t}\n"
            "\t\t},\n\n"

            "\t\t{\n"
            "\t\t\t\"name\": \"win-clang-cl-debug\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Windows\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"clang-cl\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"clang-cl\"\n"
            "\t\t\t}\n"
            "\t\t},\n"
            "\t\t{\n"
            "\t\t\t\"name\": \"win-clang-cl-release\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Windows\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"clang-cl\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"clang-cl\"\n"
            "\t\t\t}\n"
            "\t\t},\n\n"

            "\t\t{\n"
            "\t\t\t\"name\": \"linux-clang-debug\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Linux\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Debug\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"clang\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"clang++\"\n"
            "\t\t\t}\n"
            "\t\t},\n"
            "\t\t{\n"
            "\t\t\t\"name\": \"linux-clang-release\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Linux\" },\n"
            "\t\t\t\"cacheVariables\": {\n"
            "\t\t\t\t\"CMAKE_BUILD_TYPE\": \"Release\",\n"
            "\t\t\t\t\"CMAKE_C_COMPILER\": \"clang\",\n"
            "\t\t\t\t\"CMAKE_CXX_COMPILER\": \"clang++\"\n"
            "\t\t\t}\n"
            "\t\t},\n\n"

            "\t\t{\n"
            "\t\t\t\"name\": \"macos-clang-debug\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Darwin\" },\n"
            "\t\t\t\"cacheVariables\": { \"CMAKE_BUILD_TYPE\": \"Debug\" }\n"
            "\t\t},\n"
            "\t\t{\n"
            "\t\t\t\"name\": \"macos-clang-release\",\n"
            "\t\t\t\"inherits\": \"base\",\n"
            "\t\t\t\"condition\": { \"type\": \"equals\", \"lhs\": "
            "\"${hostSystemName}\", \"rhs\": \"Darwin\" },\n"
            "\t\t\t\"cacheVariables\": { \"CMAKE_BUILD_TYPE\": \"Release\" }\n"
            "\t\t}\n"
            "\t],\n\n"

            "\t\"buildPresets\": [\n"
            "\t\t{ \"name\": \"win-gcc-debug\", \"configurePreset\": "
            "\"win-gcc-debug\" },\n"
            "\t\t{ \"name\": \"win-gcc-release\", \"configurePreset\": "
            "\"win-gcc-release\" },\n"
            "\t\t{ \"name\": \"win-clang-cl-debug\", \"configurePreset\": "
            "\"win-clang-cl-debug\" },\n"
            "\t\t{ \"name\": \"win-clang-cl-release\", \"configurePreset\": "
            "\"win-clang-cl-release\" },\n"
            "\t\t{ \"name\": \"linux-clang-debug\", \"configurePreset\": "
            "\"linux-clang-debug\" },\n"
            "\t\t{ \"name\": \"linux-clang-release\", \"configurePreset\": "
            "\"linux-clang-release\" },\n"
            "\t\t{ \"name\": \"macos-clang-debug\", \"configurePreset\": "
            "\"macos-clang-debug\" },\n"
            "\t\t{ \"name\": \"macos-clang-release\", \"configurePreset\": "
            "\"macos-clang-release\" }\n"
            "\t]\n"
            "}\n");

    fclose(file);
    printf("Created: CMakePresets.json\n");
    return false;
}
