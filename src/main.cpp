#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <Windows.h>

#ifdef _WIN32
constexpr char PATH_SEPARATOR = ';';
#else
constexpr char PATH_SEPARATOR = ':';
#endif
std::vector<std::filesystem::path> pathDirectories() {
    // Putting the $PATH/%PATH% paths into a vector
    const char* path_env = std::getenv("PATH");
    std::string path_str(path_env);
    std::string temp_split;
    std::vector<std::filesystem::path> path_dirs;
    std::stringstream ss(path_str);
    while (std::getline(ss, temp_split, PATH_SEPARATOR)){
      path_dirs.push_back(temp_split);
    }
    return path_dirs;
}

std::filesystem::path findExecutable(const std::string& command) {
    auto path_dirs = pathDirectories();

    std::vector<std::string> extensions = {"", ".exe", ".bat", ".cmd"};

    for (const auto& dir : path_dirs) {
        for (const auto& ext : extensions) {
            std::filesystem::path full_path = dir / (command + ext);
            if (std::filesystem::exists(full_path) && std::filesystem::is_regular_file(full_path)) {
                std::filesystem::path curr_file = full_path.filename();
                std::filesystem::perms permissions = std::filesystem::status(curr_file).permissions();
                  bool is_exec = (permissions & (std::filesystem::perms::owner_exec)) != std::filesystem::perms::none;
                  if(is_exec){
                    return full_path;
                  }
            }
        }
    }
    return {};
}

void executeCommand(const std::string& whole_command) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    std::string command_mutable = whole_command;

    if (!CreateProcess(
        NULL,                           // No module name (use command line)
        &command_mutable[0],                // Command line
        NULL,                           // Process handle not inheritable
        NULL,                           // Thread handle not inheritable
        FALSE,                          // Set handle inheritance to FALSE
        0,                              // No creation flags
        NULL,                           // Use parent's environment block
        NULL,                           // Use parent's starting directory 
        &si,                            // Pointer to STARTUPINFO structure
        &pi)                            // Pointer to PROCESS_INFORMATION structure
    ) {
        std::cerr << "Process Failed: " << GetLastError() << '\n';
        return;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles. 
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

int main() {
  const std::vector<std::string> builtin{"echo", "type", "exit"};
  while(true){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "$ ";

    std::string command{};
    std::getline(std::cin, command);
    std::stringstream ss(command);
    std::string command_name;
    ss >> command_name;

    if(command_name == "type"){
      std::string typed_command;
      ss >> typed_command;
      if(std::find(builtin.begin(), builtin.end(), typed_command) != builtin.end()){
        std::cout << typed_command << " is a shell builtin" << '\n';
        continue;
      }else{
          try{
            std::filesystem::path executable = findExecutable(typed_command);
            if(executable != ""){
              std::cout << typed_command << " is " << executable.string() << '\n';
            }else{
              std::cerr << typed_command << ": not found" << '\n';
            }
          }catch(std::filesystem::__cxx11::filesystem_error err){}
      }
      continue;
    }else if(command_name == "echo"){
      std::cout << command.substr(5) << '\n'; 
      continue;

    }else if(command == "exit"){
      return 0;

    }else{
          try{ 
            std::filesystem::path executable = findExecutable(command_name);
            if (executable != "") {
                executeCommand(command);
            } else {
                std::cerr << command_name << ": command not found" << '\n';
          }
        }catch(std::filesystem::__cxx11::filesystem_error err){}
    }
  }
}
