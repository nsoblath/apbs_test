#include <iostream>
#include <boost/filesystem.hpp>

int main()
{
    std::cout << "Hello, world" << std::endl;
    std::cout << "Current working directory: " << boost::filesystem::current_path() << std::endl;
    return 0;
}
