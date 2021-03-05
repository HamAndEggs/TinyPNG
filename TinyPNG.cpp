
#include <fstream>
#include <iostream>
#include <array>

#include <assert.h>
#include <endian.h>

#include <zlib.h> // TODO write my own decompressor. For now use this one.

#include "TinyPNG.h"

namespace tinypng{ // Using a namespace to try to prevent name clashes as my class names are kind of obvious :)
///////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PNGChunk
{
    uint32_t mLength;
    char mChunkName[4];
    const uint8_t* mData;
    uint32_t mCRC;
    bool mOK;

    bool operator == (const char* pChunkName)const
    {
        return  mChunkName[0] == pChunkName[0] &&
                mChunkName[1] == pChunkName[1] &&
                mChunkName[2] == pChunkName[2] &&
                mChunkName[3] == pChunkName[3];
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
                mChunkName[0] = ((char*)data)[0];
                mChunkName[1] = ((char*)data)[1];
                mChunkName[2] = ((char*)data)[2];
                mChunkName[3] = ((char*)data)[3];
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
        return *((uint32_t*)mChunkName);
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
                    std::cerr << "Failed to load PNG, could read all the data for file " << pFilename << '\n';
                }
            }
        }

    }
    else if( mVerbose )
    {
        std::cerr << "Failed to load PNG " << pFilename << '\n';
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
            std::cerr << "Loading PNG failed, header is not a valid PNG header\n";
        }
        return false;
    }

    std::vector<uint8_t> compressionData;

    // Good header, now proceed to the chunks.
    size_t currentReadPos = 8;
    while(true)
    {
        PNGChunk chunk(pMemory,currentReadPos);
        if( chunk.mOK )
        {
            if( chunk == "IEND" )
            {
                // This is a bit ham fisted but as PNG does not tell you what the decompressed IDAT size is I will
                // create a decompression buffer that is used. It is set to the size of the entire image.
                std::vector<uint8_t> imageBuffer;
                imageBuffer.resize(mWidth*mHeight*8);

                std::clog << "Decompressing " << compressionData.size() << " bytes to possibly " << imageBuffer.size() << " bytes\n";

                z_stream infstream;
                infstream.zalloc = Z_NULL;
                infstream.zfree = Z_NULL;
                infstream.opaque = Z_NULL;
                // setup "b" as the input and "c" as the compressed output
                infstream.avail_in = compressionData.size(); // size of input
                infstream.next_in = (Bytef *)compressionData.data(); // input char array
                infstream.avail_out = (uInt)imageBuffer.size(); // size of output
                infstream.next_out = (Bytef *)imageBuffer.data(); // output char array

                inflateInit(&infstream);
                inflate(&infstream, Z_NO_FLUSH);
                inflateEnd(&infstream);
            
                if( infstream.total_out <= 0 )
                    return false;

                std::clog << "Decompressed size is " << infstream.total_out << "\n";
                imageBuffer.resize(infstream.total_out);

                switch( mType )
                {
                case CT_GREY_SCALE:
                    PushGreyscalePixels(imageBuffer);
                    break;

                case CT_TRUE_COLOUR:
                    PushTrueColour(imageBuffer);
                    break;

                case CT_INDEX_COLOUR:
                    PushIndexPixels(imageBuffer);
                    break;

                case CT_GREYSCALE_WITH_ALPHA:
                    PushGreyscaleAlphaPixels(imageBuffer);
                    break;

                case CT_TRUE_COLOUR_WITH_ALPHA:
                    PushTrueColourAlphaPixels(imageBuffer);
                    break;

                case CT_INVALID:
                default:
                    if( mVerbose )
                    {
                        std::cerr << "Image contains invalid image type when trying to read image data\n";
                    }
                    return false;
                }

                if( mVerbose )
                {
                    std::cerr << "Chunk IEND found, ending read\n";
                }
                return true;
            }
            else if( chunk == "IHDR" )
            {
                assert( chunk.mLength == 13 );
                if( chunk.mLength != 13 )
                {
                    if( mVerbose )
                    {
                        std::cerr << "Chunk: IHDR wrong size, should be 13 bytes, is reported as " << chunk.mLength << " bytes\n";
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
            else if( chunk == "IDAT" )
            {
                compressionData.insert(compressionData.end(),chunk.mData,chunk.mData + chunk.mLength);

            }
            else if( mVerbose )
            {
                std::clog << "Chunk: " << std::string(chunk.mChunkName,4) << "\n";
            }
        }
        else
        {
            if( mVerbose )
            {
                std::cerr << "Failed to read chunk\n";
            }
            return false;
        }
    };

    if( mVerbose )
    {
        std::cerr << "End of file hit without finding required IEND chunk, invalid file format\n";
    }

    return false;
}

bool Loader::GetRGB(std::vector<uint8_t>& rRGB)const
{
    rRGB.resize(mWidth*mHeight*3);

    uint8_t* dest = rRGB.data();

    for( uint32_t n = 0 ; n < (mWidth*mHeight) ; n++, dest += 3 )
    {
        dest[0] = mRed[n];
        dest[1] = mGreen[n];
        dest[2] = mBlue[n];
    }

    return true;
}

void Loader::PushGreyscalePixels(const std::vector<uint8_t>& pImageData)
{

}

void Loader::PushTrueColour(const std::vector<uint8_t>& pImageData)
{
    std::vector<uint8_t> filters(mHeight);


// x	the byte being filtered;
// a	the byte corresponding to x in the pixel immediately before the pixel containing x (or the byte immediately before x, when the bit depth is less than 8);
// b	the byte corresponding to x in the previous scanline;
// c	the byte corresponding to b in the pixel immediately before the pixel containing b (or the byte immediately before b, when the bit depth is less than 8).

// Type	Name	Filter Function	Reconstruction Function
// 0	None	Filt(x) = Orig(x)	Recon(x) = Filt(x)
// 1	Sub     Filt(x) = Orig(x) - Orig(a)	Recon(x) = Filt(x) + Recon(a)
// 2	Up      Filt(x) = Orig(x) - Orig(b)	Recon(x) = Filt(x) + Recon(b)
// 3	Average	Filt(x) = Orig(x) - floor((Orig(a) + Orig(b)) / 2)	Recon(x) = Filt(x) + floor((Recon(a) + Recon(b)) / 2)
// 4	Paeth	Filt(x) = Orig(x) - PaethPredictor(Orig(a), Orig(b), Orig(c))	Recon(x) = Filt(x) + PaethPredictor(Recon(a), Recon(b), Recon(c))

    auto Previous = [this](const uint8_t* pC)
    {
        const int b = *(pC - mWidth);
        const int x = *pC;
        return (uint8_t)((x + b)&0xff);
    };

    auto Average = [this](const uint8_t* pC)
    {
        const int a = *(pC - 1);
        const int b = *(pC - mWidth);
        const int x = *pC;
        return (uint8_t)(x + ((a + b) / 2));
    };

    auto Paeth = [this](const uint8_t* pC)
    {
        const int a = *(pC - 1);
        const int b = *(pC - mWidth);
        const int c = *(pC - 1 - mWidth);
        const int x = *pC;
        const int p = a + b - c;
        const int pa = std::abs(p - a);
        const int pb = std::abs(p - b);
        const int pc = std::abs(p - c);

        if( pa <= pb && pa <= pc )
        {
            return (uint8_t)(x+a);
        }
        else if( pb <= pc )
        {
            return (uint8_t)(x+b);
        }

        return (uint8_t)(x+c);
    };

    // First we need to fill in all the image data.
    // I think this will only work for 8 bit. As for 16bit I down sample to 8bit. So may need to rework the logic here.
    // First byte of the incoming data is always the filter type.
    // So record it for use later. Can't do the filtering till all data is in.
    // The spec calls it a filter but its more of a post process reconstitution as they
    // modify the data to make the compression more optimal.
    if( mBitDepth == 8 )
    {
        const uint8_t* src = pImageData.data();
        int writePos = 0;
        for( size_t y = 0 ; y < mHeight ; y++ )
        {
            filters[y] = src[0];
            src++;
            for( size_t x = 0 ; x < mWidth ; x++, src += 3, writePos++)
            {
                mRed[writePos]      = src[0];
                mGreen[writePos]    = src[1];
                mBlue[writePos]     = src[2];
                mAlpha[writePos]    = 255;
            }
        }
    }
    else if( mBitDepth == 16 )
    {
        const uint8_t* src = pImageData.data();
        int writePos = 0;
        for( size_t y = 0 ; y < mHeight ; y++ )
        {
            filters[y] = src[0];
            src++;
            for( size_t x = 0 ; x < mWidth ; x++, src += 6, writePos++)
            {
                mRed[writePos]      = src[0];
                mGreen[writePos]    = src[2];
                mBlue[writePos]     = src[4];
                mAlpha[writePos]    = 255;
            }
        }
    }
    else if( mVerbose )
    {
        std::cerr << "Invalid bit depth for true colour image, can only be 8 or 16 but we have " << mBitDepth << "\n";
    }

    // Now we'll apply the filters.
    u_int8_t* r = mRed.data();
    u_int8_t* g = mGreen.data();
    u_int8_t* b = mBlue.data();

    for( size_t y = 0 ; y < mHeight ; y++, r += mWidth, g += mWidth, b += mWidth )
    {
        switch(filters[y])
        {
        case 0:
            // No nothing
            break;
            
        case 1:
            // Add the previous value onto the current one.
            for( size_t x = 1 ; x < mWidth ; x++ )
            {
                r[x] = (uint8_t)(((int)(r[x]) + (int)(r[x-1])) & 0xff);
                g[x] = (uint8_t)(((int)(g[x]) + (int)(g[x-1])) & 0xff);
                b[x] = (uint8_t)(((int)(b[x]) + (int)(b[x-1])) & 0xff);
            }
            break;

        case 2:
            // Add the previous rows value onto the current one.
            for( size_t x = 0 ; x < mWidth ; x++ )
            {
                r[x] = Previous(r + x);
                g[x] = Previous(g + x);
                b[x] = Previous(b + x);
            }
            break;

        case 3://Average
            r[0] = Previous(r);
            g[0] = Previous(g);
            b[0] = Previous(b);
            for( size_t x = 1 ; x < mWidth ; x++ )
            {
                r[x] = Average(r + x);
                g[x] = Average(g + x);
                b[x] = Average(b + x);
            }
            break;

        case 4://paeth
            r[0] = Previous(r);
            g[0] = Previous(g);
            b[0] = Previous(b);
            for( size_t x = 1 ; x < mWidth ; x++ )
            {
                r[x] = Paeth(r + x);
                g[x] = Paeth(g + x);
                b[x] = Paeth(b + x);
            }
            break;
        }
    }
}

void Loader::PushIndexPixels(const std::vector<uint8_t>& pImageData)
{
}

void Loader::PushGreyscaleAlphaPixels(const std::vector<uint8_t>& pImageData)
{
}

void Loader::PushTrueColourAlphaPixels(const std::vector<uint8_t>& pImageData)
{

}


///////////////////////////////////////////////////////////////////////////////////////////////////////////
};// namespace tinypng
