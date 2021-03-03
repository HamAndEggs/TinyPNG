/*
   Copyright (C) 2021, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  
   
   Original code base is at https://github.com/HamAndEggs/TinyJson
   
   */

#ifndef TINY_PNG_H
#define TINY_PNG_H

#include <vector>

namespace tinypng{ // Using a namespace to try to prevent name clashes as my class names are kind of obvious :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

class Loader
{
public:

    /**
     * @brief Construct a new PNG Loader
     * 
     * @param pVerbose If true then debug information will be sent to std::log
     */
    Loader(bool pVerbose);

    /**
     * @brief Loads the PNG from file
     * 
     * @param pFilename The file to load.
     * @return true If the PNG was loaded ok.
     * @return false If file was not found our the file was corrupt.
     */
    bool LoadFromFile(const std::string& pFilename);

    /**
     * @brief Decodes the PNG that is held in memory.
     * 
     * @param pMemory The PNG as loaded from a file in memory.
     * @return true  If the PNG was loaded ok.
     * @return false If PNG was corrupt / invalid.
     */
    bool LoadFromMemory(const std::vector<uint8_t>& pMemory);

private:
    const bool mVerbose;
    int mWidth;
    int mHeight;

    // When I load I split out the channels like this to help avoid endian issues.
    // There will be supporting functions to return the data in the most popular arrangements.
    const std::vector<uint8_t> mRed,mGreen,mBlue,mAlpha;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};// namespace tinypng

#endif //TINY_PNG_H