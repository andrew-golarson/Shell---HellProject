#include <iostream>
#include <string>
#include <vector>

int main() {
  std::vector<std::string> builtin{"echo", "type", "exit"};
  while(true){
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::cout << "$ ";
    std::string command{};
    std::getline(std::cin, command);
    if(command.substr(0, 4) == "type"){
      if(find(builtin.begin(), builtin.end(), command.substr(5, command.length() - 5)) != builtin.end()){
        std::cout << command.substr(5, command.length() - 5) << " is a shell builtin" << '\n';
      }else{
        std::cerr << command << ": command not found" << '\n';
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
