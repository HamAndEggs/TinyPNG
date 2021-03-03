#include "TinyPNG.h"
#include <fstream>
#include <iostream>
#include <assert>


namespace tinypng{ // Using a namespace to try to prevent name clashes as my class names are kind of obvious :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

Loader::Loader(bool pVerbose) :
    mVerbose(pVerbose)
{

}

bool Loader::LoadFromFile(const std::string& pFilename)
{
    std::ifstream InputFile(pFilename,std::ifstream::binary);
    if( InputFile )
    {
        InputFile.seekg (0, InputFile.end);
        const size_t fileSize = InputFile.tellg();
        InputFile.seekg (0, InputFile.beg);

        if( fileSize > 0 )
        {
            std::vector<uint8_t> buffer(fileSize);

            char* FileData = (char*)buffer.data();
            assert(FileData);
            if(FileData)
            {
                InputFile.read(FileData,fileSize);
                if( InputFile )
                {
                    return LoadFromMemory(buffer);
                }
                else
                {
                    std::clog << "Failed to load PNG, could read all the data for file " << pFilename << '\n';
                }
            }
        }

    }
    else if( mVerbose )
    {
        std::clog << "Failed to load PNG " << pFilename << '\n';
    }


    return false;
}

bool Loader::LoadFromMemory(const std::vector<uint8_t>& pMemory)
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};// namespace tinypng
