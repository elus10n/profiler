#include <iostream>
#include <limits>
#include <chrono>
#include <thread>

int main()
{
    int result = 0;
    for(int i = 0;i < 500000;i++)
    {
        result++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}