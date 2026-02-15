#ifndef TEXT_PRINTER_H
#define TEXT_PRINTER_H

#include <iostream>

class TextPrinter
{
    public:

    static void welcome_print()
    {
        std::cout << "Welcome" << std::endl;
    }

    static void modes_print()
    {
        std::cout << "Modes" << std::endl;
    }

    static void metrics_print()
    {
        std::cout << "Metrics" <<std::endl;
    }
};

#endif