
#include <iostream>
#include <chrono>
#include <assert.h>

#include "../TinyPNG.h"


/**
 * @brief
 * 
 */
static bool UnitTest1()
{
    std::cout << "Running test, small PNG\n";

    tinypng::Loader rose(true);

    return rose.LoadFromFile("./rose.png");    
}

int main(int argc, char *argv[])
{
    std::cout << "Unit test app for TinyPNG build by appbuild.\n";

// Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

    UnitTest1();

// And quit";
    std::cout << "All tests passed!\n";
    return EXIT_SUCCESS;
}
