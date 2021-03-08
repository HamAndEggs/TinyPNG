
#include <iostream>
#include <chrono>
#include <vector>
#include <array>
#include <assert.h>
#include <unistd.h>

#include "../TinyPNG.h"
#include "Tiny2D.h"

/**
 * @brief
 * 
 */
static bool UnitTest1(tiny2d::FrameBuffer* FB)
{
    std::cout << "Running test, small PNG\n";

    tinypng::Loader png(true);

    const std::vector<const char*> tests = {
        "./rose.png",
        "./rose-alpha.png",
        "./rgb-8.png",
        "./rgb-16.png",
        "./rgb-alpha-8.png",
        "./rgb-alpha-16.png"
    };

    for( auto f : tests )
    {
        if( png.LoadFromFile(f) )
        {
            FB->ClearScreen(0,0,0);

            const int size = 20;

            for( int y = 0 ; y < FB->GetHeight() ; y += size )
            {
                for( int x = 0 ; x < FB->GetWidth() ; x += size )
                {
                    if( ((x/size)&1) == ((y/size)&1) )
                        FB->DrawRectangle(x,y,x+size,y+size,255,255,255,true);
                    else
                        FB->DrawRectangle(x,y,x+size,y+size,0,0,0,true);
                }
            }

            if( png.GetHasAlpha() )
            {
                std::vector<uint8_t> RGBA;

                if( png.GetRGBA(RGBA) )
                {
                    FB->BlitRGBA(RGBA.data(),0,0,png.GetWidth(),png.GetHeight());
                }
                else
                {
                    std::cout << "Failed to get RGBA for PNG " << f << "\n";
                    return false;
                }
            }
            else
            {
                std::vector<uint8_t> RGB;

                if( png.GetRGB(RGB) )
                {
                    FB->BlitRGB(RGB.data(),0,0,png.GetWidth(),png.GetHeight());
                }
                else
                {
                    std::cout << "Failed to get RGB for PNG " << f << "\n";
                    return false;
                }
            }

            FB->Present();

            sleep(2);
            FB->ClearScreen(0,0,0);
            FB->ClearScreen(0,0,0);
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

   	tiny2d::FrameBuffer* FB = tiny2d::FrameBuffer::Open(true);
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
