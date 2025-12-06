#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
constexpr char PATH_SEPARATOR = ';';
#else
constexpr char PATH_SEPARATOR = ':';
#endif

int main() {
  std::vector<std::string> builtin{"echo", "type", "exit"};
  while(true){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "$ ";

    std::string command{};
    std::getline(std::cin, command);
    if(command.substr(0, 4) == "type"){
      bool found_file = false;
      std::string typed_command = command.substr(5, command.length() - 5);
      if(std::find(builtin.begin(), builtin.end(), typed_command) != builtin.end()){
        std::cout << typed_command << " is a shell builtin" << '\n';
        continue;
      }else{
        const char* path_env = std::getenv("PATH");
        std::string path_str(path_env);
        std::string temp_split;
        std::vector<std::filesystem::path> path_dirs;
        std::stringstream ss(path_str);
        while (std::getline(ss, temp_split, PATH_SEPARATOR)){
          path_dirs.push_back(temp_split);
        }
        for(int i = 0; i<path_dirs.size(); i++){
          std::filesystem::path curr_dir = path_dirs[i];
          try{ 
            for (const auto& dirEntry : std::filesystem::directory_iterator(curr_dir)){
                std::filesystem::path curr_file = dirEntry.path();
                if(curr_file.filename() == typed_command){
                  std::cout << typed_command << " is " << curr_file.string() << '\n';
                  found_file = true;
                }
                if(found_file) break;
            }
            if(found_file) break;
          }catch(std::filesystem::__cxx11::filesystem_error err){}
        }
      }
      if(!found_file){
        std::cerr << typed_command << ": not found" << '\n';
      }
       
    }else if(command.substr(0, 4) == "echo"){
      std::cout << command.substr(5, command.length() - 5) << '\n'; 

    }else if(command == "exit"){
        return 0;

    }else{
      std::cerr << command << ": command not found" << '\n';
    }
  }

}
