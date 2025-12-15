#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <fstream>
#include "linenoise.hpp"

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
                // Windows permission checks are complex, simplified here to existence
                return full_path;
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
        NULL,                   // No module name (use command line)
        &command_mutable[0],    // Command line
        NULL,                   // Process handle not inheritable
        NULL,                   // Thread handle not inheritable
        FALSE,                  // Set handle inheritance to FALSE
        0,                      // No creation flags
        NULL,                   // Use parent's environment block
        NULL,                   // Use parent's starting directory 
        &si,                    // Pointer to STARTUPINFO structure
        &pi)                    // Pointer to PROCESS_INFORMATION structure
    ) {
        std::cerr << "Process Failed: " << GetLastError() << '\n';
        return;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

#else
// LINUX SPECIFIC
constexpr char PATH_SEPARATOR = ':';
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

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
  
void executeCommand(const std::vector<std::string>& arguments, 
                    const std::filesystem::path& std_file, 
                    const std::filesystem::path& err_file,
                    const bool std_append,
                    const bool err_append) {
    
    std::vector<char*> char_arguments;
    for(const auto& argument : arguments){
        char_arguments.push_back(const_cast<char*>(argument.c_str()));
    }
    char_arguments.push_back(nullptr);

    pid_t p = fork();
    
    if(p == 0){
        // child process

        if(!std_file.empty()) {
            int file = open(std_file.c_str(), O_WRONLY | O_CREAT | ((std_append)? O_APPEND : O_TRUNC), 0644);
            if(file < 0) { 
              perror("open stdout"); 
              exit(1); 
            }
            if(dup2(file, STDOUT_FILENO) == -1) { 
              perror("dup2 stdout"); 
              exit(1); 
            }
            close(file);
        }

        if(!err_file.empty()){
            int file = open(err_file.c_str(), O_WRONLY | O_CREAT | ((err_append)? O_APPEND : O_TRUNC), 0644);
            if(file < 0) { 
              perror("open stderr"); 
              exit(1); 
            }
            if(dup2(file, STDERR_FILENO) == -1) { 
              perror("dup2 stderr"); 
              exit(1); 
            }
            close(file);
        }

        execvp(char_arguments[0], char_arguments.data());

        std::cerr << arguments[0] << ": execution failed" << '\n';
        exit(1);
    } else if(p < 0){
        std::cerr << "Fork failed" << '\n';
    } else {
        int status;
        waitpid(p, &status, 0);
    }
}

#endif

// NON SYSTEM SPECIFIC FUNCTIONS

std::vector<std::filesystem::path> pathDirectories() {
    const char* path_env = std::getenv("PATH");
    std::string path_str(path_env ? path_env : "");
    std::string temp_split;
    std::vector<std::filesystem::path> path_dirs;
    std::stringstream ss(path_str);
    while (std::getline(ss, temp_split, PATH_SEPARATOR)){
      path_dirs.push_back(temp_split);
    }
    return path_dirs;
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
      if((c == '\'' || c == '"') && (!inside_quotes || quote_char == c)){
        inside_quotes = !inside_quotes;
        if(inside_quotes) {
            quote_char = c;
        }else{
            quote_char = 0;
        }
        continue; 
      }
      if(c == ' ' && !inside_quotes){
        if(!part_command.empty()){
          arguments.push_back(part_command);
          part_command = "";
        }
      } 
      else{
        part_command += c;
      }
  }
  if(!part_command.empty()) {
    arguments.push_back(part_command);
  }
  return arguments;
} 


int main() {
  const std::vector<std::string> builtin{"echo", "type", "exit", "pwd", "cd", "history"};
  std::vector<std::string> history_list{};
  const auto path = "history.txt";
  linenoise::SetMultiLine(true);
  linenoise::LoadHistory(path);
  linenoise::SetHistoryMaxLen(100);
  
  while(true){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::string command{};
    auto success = linenoise::Readline("$ ", command);
    if(!success) break;
    
    linenoise::AddHistory(command.c_str());
    linenoise::SaveHistory(path);
    history_list.push_back(command);    

    std::vector<std::string> parsed_args = splitCommand(command);
    if(parsed_args.empty()){
      continue;
    }
    std::string command_name = parsed_args[0];

    bool std_to_file = false;
    bool std_append = false;
    std::streambuf* cout_buff = std::cout.rdbuf();
    std::filesystem::path std_file{};

    bool err_to_file = false;
    bool err_append = false;
    std::streambuf* orig_err_buff = std::cerr.rdbuf();
    std::filesystem::path err_file{};

    std::vector<std::string> clean_args;
    for(int i = 0; i<parsed_args.size(); ++i){
        if(parsed_args[i] == ">" || parsed_args[i] == "1>" ) {
          std_to_file = true;
          if(i+1 < parsed_args.size()){
            std_file = parsed_args[i+1];
          }else{
            std::cerr << "No filename provided for stdout";
          } 
          ++i; 
        }else if(parsed_args[i] == "2>"){
          err_to_file = true;
          if(i+1 < parsed_args.size()){
            err_file = parsed_args[i+1];
          }else{
            std::cerr << "No filename provided for stderr";
          } 
          ++i;
        }else if(parsed_args[i] == ">>" || parsed_args[i] == "1>>"){
          std_to_file = true;
          std_append = true;
          if(i+1 < parsed_args.size()){
            std_file = parsed_args[i+1];
          }else{
            std::cerr << "No filename provided for stdout append";
          } 
          ++i; 
        }else if(parsed_args[i] == "2>>"){
          err_to_file = true;
          err_append = true;
          if(i+1 < parsed_args.size()){
            err_file = parsed_args[i+1];
          }else{
            std::cerr << "No filename provided for stderr append";
          } 
          ++i; 
        }else{
          clean_args.push_back(parsed_args[i]);
        }
    }

    std::stringstream ss(command);
    std::string garbage; 
    ss >> garbage;

    if(command_name == "type"){
      std::string typed_command;
      ss >> typed_command;
      
      if(std::find(builtin.begin(), builtin.end(), typed_command) != builtin.end()){
        if(!std_to_file){
          std::cout << typed_command << " is a shell builtin" << '\n';
        }else{
          std::ofstream file(std_file, ((std_append) ? std::ios::app : std::ios::trunc));
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
                std::ofstream file(std_file, ((std_append) ? std::ios::app : std::ios::trunc));
                file << typed_command << " is " << executable.string() << '\n';
                file.close();
              }
            }else{
              std::cerr << typed_command << ": not found" << '\n';
            }
          }catch(std::filesystem::__cxx11::filesystem_error err){}
      }

    }else if(command_name == "echo"){
      std::string message;

      for(int i = 1; i < clean_args.size(); ++i) {
        if(!message.empty()) message += " ";
        message += clean_args[i];
      }

      if(!std_to_file){
        std::cout << message << '\n'; 
      }else{
        std::ofstream file(std_file, ((std_append) ? std::ios::app : std::ios::trunc));
        file << message << '\n';
        file.close();
      }
      if(err_to_file){
        std::ofstream file(err_file, ((err_append)? std::ios::app : std::ios::trunc));
        file.close();
      }

    }else if(command_name == "exit"){
      return 0;

    }else if(command_name == "pwd"){
      if(!std_to_file){
        std::cout << std::filesystem::current_path().string() << '\n';
      }else{
        std::ofstream file(std_file, ((std_append) ? std::ios::app : std::ios::trunc));
        file << std::filesystem::current_path().string() << '\n';
        file.close();
      }
      
    }else if(command_name == "history"){
      int limit{};
      if(clean_args.size() > 1){
        try{
          limit = history_list.size() - std::stoi(clean_args[1]);
          if(limit < 0){
            limit = 0;
            throw;
          }
        }catch(std::exception &err){
          std::cerr << "Invalid limiting history input, has to be a positive integer";
        }
      }
      for(int i = limit; i<history_list.size(); i++){
        std::cout << i+1 << "  " << history_list[i] << '\n';
      }
    }else if(command_name == "cd"){
      std::filesystem::path cd_path;

      if(clean_args.size() > 1) cd_path = clean_args[1];

      std::ofstream file{err_file, ((err_append)? std::ios::app : std::ios::trunc)};
      if(err_to_file){
        std::cerr.rdbuf(file.rdbuf());
      }

      if(cd_path == "~"){
        #ifdef _WIN32
          const char* home_dir = std::getenv("USERPROFILE");
          if(home_dir) std::filesystem::current_path(home_dir);
          if(err_to_file){
            std::cerr.rdbuf(orig_err_buff);
            file.close(); 
          }
        #else
          const char* home_dir = std::getenv("HOME");
          if(home_dir) std::filesystem::current_path(home_dir);
          if(err_to_file){
            std::cerr.rdbuf(orig_err_buff);
            file.close(); 
          }
        #endif
      }else{
        try{
          std::filesystem::current_path(cd_path);
          if(err_to_file){
            std::cerr.rdbuf(orig_err_buff);
            file.close(); 
          }
        }catch(std::filesystem::filesystem_error err){
          std::cerr << "cd: " << cd_path.string() << ": No such file or directory" << '\n';
          if(err_to_file){
            std::cerr.rdbuf(orig_err_buff);
            file.close(); 
          }
        }
      }

    }else{
        try{
            std::filesystem::path executable = findExecutable(command_name);
            if (executable != "") {
              #ifdef _WIN32
                if(std_to_file){
                  std::ofstream file(std_file, ((std_append) ? std::ios::app : std::ios::trunc));
                  std::cout.rdbuf(file.rdbuf());
                  executeCommand(command);
                  file.close();
                }else if(err_to_file){
                  std::ofstream file(err_file, ((err_append)? std::ios::app : std::ios::trunc));
                  std::cerr.rdbuf(file.rdbuf());         
                  executeCommand(command);
                  file.close();
                }else{
                  executeCommand(command);
                }
                std::cerr.rdbuf(orig_err_buff);
                std::cout.rdbuf(cout_buff);
              #else
                executeCommand(clean_args, std_file, err_file, std_append, err_append);
              #endif
            }else{
              if(err_to_file){
                std::ofstream file(err_file, ((err_append)? std::ios::app : std::ios::trunc));
                file << command_name << ": command not found" << '\n';
                file.close();
              }else{
                std::cerr << command_name << ": command not found" << '\n';
              }
          }
        }catch(std::filesystem::__cxx11::filesystem_error err){}
    }
  }
}