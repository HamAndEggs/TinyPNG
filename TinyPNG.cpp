#include "TinyPNG.h"
#include <fstream>
#include <iostream>
#include <assert.h>
#include <endian.h>

namespace tinypng{ // Using a namespace to try to prevent name clashes as my class names are kind of obvious :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PNGChunk
{
    uint32_t mLength;
    char mType[4];
    const uint8_t* mData;
    uint32_t mCRC;
    bool mOK;

    bool operator == (const char* pChunkName)const
    {
        return  mType[0] == pChunkName[0] &&
                mType[1] == pChunkName[1] &&
                mType[2] == pChunkName[2] &&
                mType[3] == pChunkName[3];
    }

    PNGChunk(const std::vector<uint8_t>& pMemory,size_t& pCurrentPosition) :
        mLength(0),
        mData(nullptr),
        mOK(false)
    {
        const uint8_t* data = pMemory.data() + pCurrentPosition;
        const uint8_t* end = pMemory.data() + pMemory.size();

        assert( data + 4 <= end );
        if( data + 4 <= end )
        {
            mLength = be32toh(*((uint32_t*)data));
            data += 4;
            assert( data + 4 <= end );
            if( data + 4 <= end )
            {
                mType[0] = ((char*)data)[0];
                mType[1] = ((char*)data)[1];
                mType[2] = ((char*)data)[2];
                mType[3] = ((char*)data)[3];
                data += 4;
                assert( data + mLength <= end );
                if( data + mLength <= end )
                {
                    mData = data;
                    data += mLength;

                    assert( data + 4 <= end );
                    if( data + 4 <= end )
                    {
                        pCurrentPosition += 12 + mLength;
                        mCRC = be32toh(*((uint32_t*)data));
                        mOK = true;// TODO: Add the CRC check.
                    }
                }
            }
        }
    }

    uint32_t GetFourCC()
    {
        return *((uint32_t*)mType);
    }
};

Loader::Loader(bool pVerbose) :
    mVerbose(pVerbose),
    mWidth(0),
    mHeight(0),
    mBitDepth(0),
    mType(CT_INVALID)
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
    // This is a bit of a slow way to check the header, but this is endian safe.
    if( pMemory[0] != 0x89 || // Has the high bit set to detect transmission systems that do not support 8-bit data and to reduce the chance that a text file is mistakenly interpreted as a PNG, or vice versa.
        pMemory[1] != 0x50 || pMemory[2] != 0x4E || pMemory[3] != 0x47 || // In ASCII, the letters PNG, allowing a person to identify the format easily if it is viewed in a text editor.
        pMemory[4] != 0x0D || pMemory[5] != 0x0A || // A DOS-style line ending (CRLF) to detect DOS-Unix line ending conversion of the data.
        pMemory[6] != 0x1A || // A byte that stops display of the file under DOS when the command type has been used—the end-of-file character.
        pMemory[7] != 0x0A ) // A Unix-style line ending (LF) to detect Unix-DOS line ending conversion.
    {
        if( mVerbose )
        {
            std::clog << "Loading PNG failed, header is not a valid PNG header\n";
        }
        return false;
    }

    // Good header, now proceed to the chunks.
    size_t currentPos = 8;
    while(true)
    {
        PNGChunk chunk(pMemory,currentPos);
        if( chunk.mOK )
        {
            if( chunk == "IEND" )
            {
                if( mVerbose )
                {
                    std::clog << "Chunk IEND found, ending read\n";
                    return true;
                }
            }
            else if( chunk == "IHDR" )
            {
                assert( chunk.mLength == 13 );
                if( chunk.mLength != 13 )
                {
                    if( mVerbose )
                    {
                        std::clog << "Chunk: IHDR wrong size, should be 13 bytes, is reported as " << chunk.mLength << " bytes\n";
                    }                    
                    return false;
                }

                const uint8_t* data = chunk.mData;

                mWidth = be32toh(*((uint32_t*)data));data += 4;
                mHeight = be32toh(*((uint32_t*)data));data += 4;
                mBitDepth = (int)(data[0]);
                mType = (PNGColourType)(data[1]);
                mCompressionMethod = (PNGColourType)(data[2]);
                mFilterMethod = (int)(data[3]);
                mInterlaceMethod = (int)(data[4]);

                if( mVerbose )
                {
                    std::clog << "Image Header:" <<
                                " Width " << mWidth <<
                                " Height " << mHeight <<
                                " Bit Depth " << mBitDepth <<
                                " Colour Type " << mType <<
                                " Compression Method " << mCompressionMethod <<
                                " Filter Method " << mFilterMethod <<                     
                                " Interlace Method " << mInterlaceMethod
                                << "\n";
                }

                // Prepare the image buffers.
                // Internally I store as 8 bit per channel.
                // So for 16bit channel data we'll be loosing data. 
                // This is not meant to be an all signing all dancing implementation.
                // Just a tiny one. :)
                mRed.resize(mWidth*mHeight);
                mGreen.resize(mWidth*mHeight);
                mBlue.resize(mWidth*mHeight);
                mAlpha.resize(mWidth*mHeight);

            }
            else if( mVerbose )
            {
                std::clog << "Chunk: " << std::string(chunk.mType,4) << "\n";
            }

        }
        else
        {
            return false;
        }
    };

    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
};// namespace tinypng
