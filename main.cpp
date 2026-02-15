#include <iostream>
#include <functional>

// #include "./console_interface/user_interface_c.h"
#include "./console_interface/configurator.h"


int main() 
{
    // ConsoleInterface interface;
    // interface.run();
    Configurator configurator;
    Configuration cfg = configurator.get_configuration();
    std::cout << cfg;
}