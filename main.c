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
bool create_ps1_files(const char* project_name);
bool create_bat_files(const char* project_name);
void print_help(const char* program_name);

int main(int argc, const char* argv[]) {
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

    // Handle command line arguments
    bool has_error = false;
    if (argc >= 2) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        } else if (strcmp(argv[1], "both") == 0) {
            bool ps1_error = create_ps1_files(project_name);
            bool bat_error = create_bat_files(project_name);
            has_error = ps1_error || bat_error;
        } else if (strcmp(argv[1], "bat") == 0) {
            has_error = create_bat_files(project_name);
        } else {
            fprintf(stderr, "Error: Unknown option '%s'\n\n", argv[1]);
            print_help(argv[0]);
            return 1;
        }
    } else {
        has_error = create_ps1_files(project_name);
    }

    if (has_error) {
        return 1;
    }

    if (file_exists("CMakeLists.txt")) {
        fprintf(
            stderr,
            "Error: CMakeLists.txt already exists in the current directory\n");
        return 1;
    }

    if (create_cmakelists_file(project_name)) {
        return 1;
    }

    return 0;
}

void print_help(const char* program_name) {
    printf("Usage: %s [OPTION]\n", program_name);
    printf("Generate CMakeLists.txt and build scripts for a C/C++ project\n\n");
    printf("Options:\n");
    printf("  (no args)     Create PowerShell scripts only (.ps1)\n");
    printf("  both          Create both PowerShell and Batch scripts\n");
    printf("  bat           Create Batch scripts only (.bat)\n");
    printf("  -h, --help    Display this help message\n");
}

bool create_cmakelists_file(const char* project_name) {
    FILE* file = fopen("CMakeLists.txt", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to create CMakeLists.txt\n");
        return true;
    }

    fprintf(file, "cmake_minimum_required(VERSION 3.20)\n\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# Project\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "project(%s\n", project_name);
    fprintf(file, "\tLANGUAGES C CXX\n)\n");
    fprintf(file, "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n");

    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "# C Standard\n");
    fprintf(file, "# --------------------------------------\n");
    fprintf(file, "set(CMAKE_C_STANDARD 17)\n");
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

    fprintf(file,
            "link_directories(\"${PROJECT_SOURCE_DIR}/external/lib\")\n\n");

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
    fprintf(file, "\t\ttraget_compile_options(%s PRIVATE -Wshadow)\n",
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

bool create_ps1_files(const char* project_name) {
    // Check if files already exist
    if (file_exists("build.ps1")) {
        fprintf(stderr, "Error: build.ps1 already exists\n");
        return true;
    }
    if (file_exists("run.ps1")) {
        fprintf(stderr, "Error: run.ps1 already exists\n");
        return true;
    }
    if (file_exists("clean.ps1")) {
        fprintf(stderr, "Error: clean.ps1 already exists\n");
        return true;
    }

    // Create build.ps1
    FILE* file1 = fopen("build.ps1", "w");
    if (file1 == NULL) {
        fprintf(stderr, "Error: Failed to create build.ps1\n");
        return true;
    }

    fprintf(file1, "param(\n");
    fprintf(file1, "\t[ValidateSet(\"gcc\", \"clang\", \"clang-cl\")]\n");
    fprintf(file1, "\t[string]$compiler = \"gcc\"\n)\n\n");
    fprintf(file1, "$exeDir = \"build\"\n");
    fprintf(file1, "$exePath = Join-Path $exeDir \"%s.exe\"\n\n", project_name);
    fprintf(
        file1,
        "Write-Host \"Building with $compiler...\" -ForegroundColor Cyan\n\n");
    fprintf(file1, "try {\n");
    fprintf(file1, "\tif ($compiler -eq \"clang\") {\n");
    fprintf(file1, "\t\tcmake -S . -B $exeDir -G Ninja `\n");
    fprintf(file1, "\t\t\t-DCMAKE_C_COMPILER=clang `\n");
    fprintf(file1, "\t\t\t-DCMAKE_CXX_COMPILER=clang++\n\t}\n");
    fprintf(file1, "\telseif ($compiler -eq \"clang-cl\") {\n");
    fprintf(file1, "\t\tcmake -S . -B $exeDir -G Ninja `\n");
    fprintf(file1, "\t\t\t-DCMAKE_C_COMPILER=clang-cl `\n");
    fprintf(file1, "\t\t\t-DCMAKE_CXX_COMPILER=clang-cl `\n");
    fprintf(file1, "\t\t\t-DCMAKE_LINKER=link\n");
    fprintf(file1, "\t} else {\n");
    fprintf(file1, "\t\tcmake -S . -B $exeDir -G Ninja `\n");
    fprintf(file1, "\t\t\t-DCMAKE_C_COMPILER=gcc `\n");
    fprintf(file1, "\t\t\t-DCMAKE_CXX_COMPILER=g++\n\t}\n\n");
    fprintf(file1,
            "\tif ($LASTEXITCODE -ne 0) { throw \"CMake configuration failed\" "
            "}\n\n");
    fprintf(file1, "\tcmake --build $exeDir\n");
    fprintf(file1, "\tif ($LASTEXITCODE -ne 0) { throw \"Build failed\" }\n");
    fprintf(file1, "}\n");
    fprintf(file1, "catch {\n");
    fprintf(file1, "\tWrite-Host \"Build failed: $_\" -ForegroundColor Red\n");
    fprintf(file1, "\texit 1\n");
    fprintf(file1, "}\n\n");
    fprintf(file1, "if (-Not (Test-Path $exePath)) {\n");
    fprintf(file1,
            "\tWrite-Host \"Build finished but %s.exe not found!\" "
            "-ForegroundColor Red\n",
            project_name);
    fprintf(file1, "\texit 1\n");
    fprintf(file1, "}\n\n");
    fprintf(file1, "Write-Host \"Build succeeded!\" -ForegroundColor Green\n");
    fclose(file1);
    printf("Created: build.ps1\n");

    // Create run.ps1
    FILE* file2 = fopen("run.ps1", "w");
    if (file2 == NULL) {
        fprintf(stderr, "Error: Failed to create run.ps1\n");
        return true;
    }

    fprintf(file2, "param(\n");
    fprintf(file2, "\t[ValidateSet(\"gcc\", \"clang\", \"clang-cl\")]\n");
    fprintf(file2, "\t[string]$compiler = \"gcc\"\n)\n\n");
    fprintf(file2, "$exePath = Join-Path \"build\" \"%s.exe\"\n\n",
            project_name);
    fprintf(file2, "if (-Not (Test-Path $exePath)) {\n");
    fprintf(file2, "\tWrite-Host \"Building...\" -ForegroundColor Yellow\n");
    fprintf(file2, "\t& ./build.ps1 -compiler $compiler\n");
    fprintf(file2, "}\n\n");
    fprintf(file2, "if (-Not (Test-Path $exePath)) {\n");
    fprintf(file2,
            "\tWrite-Host \"Cannot run: executable not found!\" "
            "-ForegroundColor Red\n");
    fprintf(file2, "\texit 1\n");
    fprintf(file2, "}\n\n");
    fprintf(file2, "Write-Host \"Running %s...\" -ForegroundColor Cyan\n",
            project_name);
    fprintf(file2, "try {\n");
    fprintf(file2, "\t& $exePath\n");
    fprintf(file2, "}\n");
    fprintf(file2, "catch {\n");
    fprintf(
        file2,
        "\tWrite-Host \"Error during execution: $_\" -ForegroundColor Red\n");
    fprintf(file2, "\texit 1\n");
    fprintf(file2, "}\n");
    fclose(file2);
    printf("Created: run.ps1\n");

    // Create clean.ps1
    FILE* file3 = fopen("clean.ps1", "w");
    if (file3 == NULL) {
        fprintf(stderr, "Error: Failed to create clean.ps1\n");
        return true;
    }

    fprintf(
        file3,
        "Write-Host \"Cleaning build directory...\" -ForegroundColor Yellow\n");
    fprintf(file3,
            "Remove-Item -Path build -Recurse -Force -ErrorAction "
            "SilentlyContinue\n");
    fprintf(file3, "Write-Host \"Done!\" -ForegroundColor Green\n");
    fclose(file3);
    printf("Created: clean.ps1\n");

    return false;
}

bool create_bat_files(const char* project_name) {
    // Check if files already exist
    if (file_exists("build.bat")) {
        fprintf(stderr, "Error: build.bat already exists\n");
        return true;
    }
    if (file_exists("run.bat")) {
        fprintf(stderr, "Error: run.bat already exists\n");
        return true;
    }
    if (file_exists("clean.bat")) {
        fprintf(stderr, "Error: clean.bat already exists\n");
        return true;
    }

    // Create build.bat
    FILE* file1 = fopen("build.bat", "w");
    if (file1 == NULL) {
        fprintf(stderr, "Error: Failed to create build.bat\n");
        return true;
    }

    fprintf(file1, "@echo off\n");
    fprintf(file1, "setlocal enabledelayedexpansion\n\n");
    fprintf(file1, "REM Default\n");
    fprintf(file1, "set COMPILER=gcc\n");
    fprintf(file1, "set CXX_COMPILER=g++\n\n");
    fprintf(file1, "REM Parse argument\n");
    fprintf(file1, "if \"%%1\"==\"clang\" (\n");
    fprintf(file1, "\tset COMPILER=clang\n");
    fprintf(file1, "\tset CXX_COMPILER=clang++\n");
    fprintf(file1, ") else if \"%%1\"==\"clang-cl\" (\n");
    fprintf(file1, "\tset COMPILER=clang-cl\n");
    fprintf(file1, "\tset CXX_COMPILER=clang-cl\n");
    fprintf(file1, ")\n\n");
    fprintf(file1, "echo Building with %%COMPILER%%...\n");
    fprintf(file1,
            "cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=%%COMPILER%% "
            "-DCMAKE_CXX_COMPILER=%%CXX_COMPILER%%\n\n");
    fprintf(file1, "if errorlevel 1 (\n");
    fprintf(file1, "\techo CMake configuration failed!\n");
    fprintf(file1, "\texit /b 1\n");
    fprintf(file1, ")\n\n");
    fprintf(file1, "cmake --build build\n");
    fprintf(file1, "if errorlevel 1 (\n");
    fprintf(file1, "\techo Build failed!\n");
    fprintf(file1, "\texit /b 1\n");
    fprintf(file1, ")\n\n");
    fprintf(file1, "if not exist build\\%s.exe (\n", project_name);
    fprintf(file1, "\techo Build finished but %s.exe not found!\n",
            project_name);
    fprintf(file1, "\texit /b 1\n");
    fprintf(file1, ")\n\n");
    fprintf(file1, "echo Build succeeded!\n");
    fclose(file1);
    printf("Created: build.bat\n");

    // Create run.bat
    FILE* file2 = fopen("run.bat", "w");
    if (file2 == NULL) {
        fprintf(stderr, "Error: Failed to create run.bat\n");
        return true;
    }

    fprintf(file2, "@echo off\n");
    fprintf(file2, "setlocal\n\n");
    fprintf(file2, "REM Default\n");
    fprintf(file2, "set COMPILER=gcc\n");
    fprintf(file2, "if \"%%1\"==\"clang\" set COMPILER=clang\n");
    fprintf(file2, "if \"%%1\"==\"clang-cl\" set COMPILER=clang-cl\n\n");
    fprintf(file2, "if not exist build\\%s.exe (\n", project_name);
    fprintf(file2, "\techo Building...\n");
    fprintf(file2, "\tcall build.bat %%COMPILER%%\n");
    fprintf(file2, ")\n\n");
    fprintf(file2, "if not exist build\\%s.exe (\n", project_name);
    fprintf(file2, "\techo Cannot run: executable not found!\n");
    fprintf(file2, "\texit /b 1\n");
    fprintf(file2, ")\n\n");
    fprintf(file2, "echo Running %s...\n", project_name);
    fprintf(file2, "build\\%s.exe\n", project_name);
    fclose(file2);
    printf("Created: run.bat\n");

    // Create clean.bat
    FILE* file3 = fopen("clean.bat", "w");
    if (file3 == NULL) {
        fprintf(stderr, "Error: Failed to create clean.bat\n");
        return true;
    }

    fprintf(file3, "@echo off\n");
    fprintf(file3, "echo Cleaning build directory...\n");
    fprintf(file3, "if exist build rmdir /s /q build\n");
    fprintf(file3, "echo Done!\n");
    fclose(file3);
    printf("Created: clean.bat\n");

    return false;
}
