
#include <iostream>
#include <chrono>
#include <vector>
#include <array>
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

    tinypng::Loader png(true);

    const std::array<const char*,3> tests = {
        "./rose.png",
        "./rgb-8.png",
        "./rgb-16.png",
    };

    for( auto f : tests )
    {
        if( png.LoadFromFile(f) )
        {
            FB->ClearScreen(0,0,0);

            std::vector<uint8_t> RGB;

            if( png.GetRGB(RGB) )
            {
                FB->BlitRGB24(RGB.data(),0,0,png.GetWidth(),png.GetHeight());
            }
            else
            {
                std::cout << "Failed to get RGB for PNG " << f << "\n";
                return false;
            }

            FB->Present();

            sleep(5);
        }
        else
        {
            std::cout << "Failed to load PNG " << f << "\n";
            return false;
        }
    }

    return true;
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
