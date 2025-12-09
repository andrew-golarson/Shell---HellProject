#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <fstream>

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
      std::cerr << "Fork failed" << '\n';
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
  const std::vector<std::string> builtin{"echo", "type", "exit", "pwd", "cd"};
  while(true){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "$ ";

    std::string command{};
    std::getline(std::cin, command);
    std::stringstream ss(command);
    std::string command_name;
    ss >> command_name;
    bool std_to_file = false;

    if(command_name == "type"){
      std::string typed_command;
      ss >> typed_command;
      if(ss.rdbuf()->in_avail() != 0){
        std::string temp;
        ss >> temp;
        if(temp == ">" || temp == "1>"){
          std_to_file = true;
        }
      }
      
      if(std::find(builtin.begin(), builtin.end(), typed_command) != builtin.end()){
        if(!std_to_file){
          std::cout << typed_command << " is a shell builtin" << '\n';
        }else{
          std::string std_filename;
          ss >> std_filename;
          std::ofstream file(std_filename);
          file << typed_command << " is a shell builtin" << '\n';
          file.close();
        }
        continue;
      }else{
          try{
            std::filesystem::path executable = findExecutable(typed_command);
            if(executable != ""){
              if(!std_to_file){
                std::cout << typed_command << " is " << executable.string() << '\n';
              }else{               
                std::string std_filename;
                ss >> std_filename;
                std::ofstream file(std_filename);
                file << typed_command << " is " << executable.string() << '\n';
                file.close();
              }
            }else{
              std::cerr << typed_command << ": not found" << '\n';
            }
          }catch(std::filesystem::__cxx11::filesystem_error err){}
      }

    }else if(command_name == "echo"){
      bool erase_one_more_space = false;
      std::string temp{};
      std::string message;
      while (ss >> temp) {
      if (temp == ">") {
        std_to_file = true;
        break;
      }else if(temp == "1>"){
        std_to_file = true;
        erase_one_more_space = true;
        break;
      }else{
        if(!message.empty()){
          message += " ";
        }
        message += temp;
      }
      }
      if(!std_to_file){
        std::cout << command.substr(5) << '\n'; 
      }else{
        std::string std_filename;
        if(ss >> std_filename){
        }else{
          std::cerr << "No filename provided";
          continue;
        }
        std::ofstream file(std_filename);
        file << message << '\n';
        file.close();
      }

    }else if(command_name == "exit"){
      return 0;

    }else if(command_name == "pwd"){
      if(ss.rdbuf()->in_avail() != 0){
        std::string temp;
        ss >> temp;
        if(temp == ">" || temp == "1>"){
          std_to_file = true;
        }
      }
      if(!std_to_file){
        std::cout << std::filesystem::current_path().string() << '\n';
      }else{
        std::string std_filename;
        ss >> std_filename;
        std::ofstream file(std_filename);
        file << std::filesystem::current_path().string() << '\n';
        file.close();
      }
      
    }else if(command_name == "cd"){
      std::filesystem::path cd_path;
      ss >> cd_path;

      if(cd_path == "~"){
        #ifdef _WIN32
          const char* home_dir = std::getenv("USERPROFILE");
          std::filesystem::current_path(home_dir);
        #else
          const char* home_dir = std::getenv("HOME");
          std::filesystem::current_path(home_dir);
        #endif
      }else{
        try{
          std::filesystem::current_path(cd_path);
        }catch(std::filesystem::filesystem_error err){
          std::cerr << "cd: " << cd_path.string() << ": No such file or directory" << '\n';
        }
      }

    }else{
        try{ 
            std::filesystem::path executable = findExecutable(command_name);
            std::string temp{};
            std::string filename{};
            while(ss >> temp){
              if(temp == ">" || temp== "1>"){
                auto it = command.rfind(">");
                if(ss>>filename){
                }else{
                  std::cerr << "No filename provided";
                }
                command.erase(it);
                std_to_file = true;
                break;
              }
            }
            if (executable != "") {
              #ifdef _WIN32
                auto cout_buff = std::cout.rdbuf();
                if(std_to_file){
                  std::ofstream file(filename);
                  std::cout.rdbuf(file.rdbuf());
                  executeCommand(command);
                }else{
                  executeCommand(command);
                }
                std::cout.rdbuf(cout_buff);
              #else
                auto cout_buff = std::cout.rdbuf(); 
                if(std_to_file){
                  std::ofstream file(filename);
                  std::cout.rdbuf(file.rdbuf());
                  executeCommand(splitCommand(command));
                }else{
                  executeCommand(splitCommand(command));
                }
                std::cout.rdbuf(cout_buff);
              #endif
            } else {
                std::cerr << command_name << ": command not found" << '\n';
          }
        }catch(std::filesystem::__cxx11::filesystem_error err){}
    }
  }
}
