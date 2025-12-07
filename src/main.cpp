#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <algorithm>

std::vector<std::filesystem::path> pathDirectories();

#ifdef _WIN32
// WINDOWS SPECIFIC
#include <windows.h>
constexpr char PATH_SEPARATOR = ';';

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
        &command_mutable[0],            // Command line
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

    // Wait until child process exits
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#else
// LINUX SPECIFIC
constexpr char PATH_SEPARATOR = ':';
#include <unistd.h>
#include <sys/wait.h>
std::filesystem::path findExecutable(const std::string& whole_command) {
    if (whole_command.find('/') != std::string::npos) {
        if (access(whole_command.c_str(), X_OK) == 0) return whole_command;
        return {};
    }

    auto path_dirs = pathDirectories();
    for (const auto& dir : path_dirs) {
        std::filesystem::path full_path = dir / whole_command;
            if (access(full_path.c_str(), X_OK) == 0) {
                std::filesystem::path curr_file = full_path.filename();
                std::filesystem::perms permissions = std::filesystem::status(curr_file).permissions();
                  bool is_exec = (permissions & (std::filesystem::perms::owner_exec)) != std::filesystem::perms::none;
                  if(is_exec){
                    return full_path;
                  }
            }
    }
    return {};
  }
  std::vector<std::string> splitCommand(const std::string& whole_command){
  if(whole_command.empty()){
    return {};
  }
  std::string part_command;
  std::vector<std::string> arguments;
  bool inside_quotes = false;
  char quote_char = 0; // Tracks if we are in ' or "
  for(char c : whole_command){
        if ((c == '\'' || c == '"') && (!inside_quotes || quote_char == c)) {
            inside_quotes = !inside_quotes;
            if (inside_quotes) {
                quote_char = c;
            } else {
                quote_char = 0;
            }
            continue; 
        }
        if (c == ' ' && !inside_quotes) {
            arguments.push_back(part_command);
            part_command = "";
        } 
        else {
            part_command += c;
        }
    }
    if (!part_command.empty()) {
        arguments.push_back(part_command);
    }
    return arguments;
  } 
  
void executeCommand(const std::vector<std::string>& arguments){
    std::vector<char*> char_arguments;
    for(const auto& argument : arguments){
      char_arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    char_arguments.push_back(nullptr);
    pid_t p = fork();
    
    if(p == 0){
      execvp(char_arguments[0], char_arguments.data());
    }else if(p<0){
      std::cerr << "Fork fail" << '\n';
    }else{
      int status;
      waitpid(p, &status, 0);
    }
}

#endif

// NON SYSTEM SPECIFIC FUNCTIONS

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





int main() {
  const std::vector<std::string> builtin{"echo", "type", "exit", "pwd"};
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

    }else if(command_name == "exit"){
      return 0;

    }else if(command_name == "pwd"){
      std::cout << std::filesystem::current_path() << '\n';

    }else{
      
        try{ 
            std::filesystem::path executable = findExecutable(command_name);
            if (executable != "") {
              #ifdef _WIN32
                executeCommand(command);
              #else
                executeCommand(splitCommand(command));
              #endif
            } else {
                std::cerr << command_name << ": command not found" << '\n';
          }
        }catch(std::filesystem::__cxx11::filesystem_error err){}
    }
  }
}
