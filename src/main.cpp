#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;
  while(true){
    std::cout << "$ ";
    std::string command{};
    std::getline(std::cin, command);
    if(command.substr(0, 4) == "echo"){
      std::cout << command.substr(5, command.length() - 4) << '\n'; 
    }else if(command == "exit"){
        return 0;
    }else{
      std::cerr << command << ": command not found" << '\n';
    }
  }

}
