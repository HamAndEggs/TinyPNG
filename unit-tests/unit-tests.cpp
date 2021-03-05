
#include <iostream>
#include <chrono>
#include <vector>
#include <assert.h>
#include <unistd.h>

#include "../TinyPNG.h"
#include "framebuffer.h"

/**
 * @brief
 * 
 */
static bool UnitTest1(FBIO::FrameBuffer* FB)
{
    std::cout << "Running test, small PNG\n";

    tinypng::Loader rose(true);

    if( rose.LoadFromFile("./rose.png") )
    {
        FB->ClearScreen(0,0,0);

        std::vector<uint8_t> RGB;

        if( rose.GetRGB(RGB) )
        {
            FB->BlitRGB24(RGB.data(),0,0,rose.GetWidth(),rose.GetHeight());
        }

        FB->Present();

        sleep(5);
        return true;
    }

    return false;
}

int main(int argc, char *argv[])
{
    std::cout << "Unit test app for TinyPNG build by appbuild.\n";

    // Display the constants defined by app build. \n";
    std::cout << "Application Version " << APP_VERSION << '\n';
    std::cout << "Build date and time " << APP_BUILD_DATE_TIME << '\n';
    std::cout << "Build date " << APP_BUILD_DATE << '\n';
    std::cout << "Build time " << APP_BUILD_TIME << '\n';

   	FBIO::FrameBuffer* FB = FBIO::FrameBuffer::Open(true,true);
	if( !FB )
		return EXIT_FAILURE;

    bool AllGood = true;

    FB->ClearScreen(0,0,0);
    FB->Present();

    if( UnitTest1(FB) == false )
    {
        std::cerr << "Unit test 1 failed\n";
        AllGood = false;
    }

	// Stop monitor burn in...
	FB->ClearScreen(0,0,0);
	
	delete FB;

    // And quit
    if( AllGood )
    {
        std::cout << "All tests passed!\n";
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}
