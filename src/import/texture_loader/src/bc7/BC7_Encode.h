//===============================================================================
// Copyright (c) 2007-2016  Advanced Micro Devices, Inc. All rights reserved.
// Copyright (c) 2004-2006 ATI Technologies Inc.
//===============================================================================
// [# filter:bc7 #]
// [# pch:disabled #]
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef _BC7_ENCODE_H_
#define _BC7_ENCODE_H_

#include <float.h>
#include "BC7_Definitions.h"
//#include "debug.h"

//#include <mutex>
#include <algorithm>

// Threshold quality below which we will always run fast quality and shaking
// Self note: User should be able to set this?
extern double g_qFAST_THRESHOLD;
extern double g_HIGHQULITY_THRESHOLD;

class BC7BlockEncoder
{
public:

    BC7BlockEncoder(unsigned int validModeMask,
        bool  imageNeedsAlpha,
                    double quality,
        bool colourRestrict,
        bool alphaRestrict,
                    double performance = 1.0
                    )
                    {
                        // Bug check : ModeMask must be > 0
                        if (validModeMask <= 0) 
                            m_validModeMask = 0xCF;
                        else
                            m_validModeMask = validModeMask;

                        m_quality            = std::min<double>(1.0, std::max<double>(quality,0.0));
                        m_performance        = std::min<double>(1.0, std::max<double>(performance,0.0));
                        m_imageNeedsAlpha    = imageNeedsAlpha;
                        m_smallestError      = DBL_MAX;
                        m_largestError       = 0.0;
                        m_colourRestrict     = colourRestrict;
                        m_alphaRestrict      = alphaRestrict;
                        
                        m_quantizerRangeThreshold  = 255 * m_performance;

                        if(m_quality < g_qFAST_THRESHOLD) // Make sure this is below 0.5 since we are x2 below.
                        {
                            m_shakerRangeThreshold = 0.;
                        
                            // Scale m_quality to be a linar range 0 to 1 in this section 
                            // to maximize quality with fast performance...
                            m_errorThreshold = 256. * (1.0 - ((m_quality*2.0)/g_qFAST_THRESHOLD));
                            // Limit the size of the partition search space based on Quality
                            m_partitionSearchSize = std::max<double>( (1.0/16.0) , ((m_quality*2.0) / g_qFAST_THRESHOLD));
                        }
                        else
                        {
                            // m_qaulity = set the quality user want to see on encoding 
                            // higher values will produce better encoding results.
                            // m_performance  - sets a perfoamce level for a specified quality level 


                            if(m_quality < g_HIGHQULITY_THRESHOLD)
                            {
                                m_shakerRangeThreshold  = 255 * (m_quality / 10);                    // gain  performance within FAST_THRESHOLD and HIGHQULITY_THRESHOLD range
                                m_errorThreshold = 256. * (1.0 - (m_quality/g_qFAST_THRESHOLD));
                                // Limit the size of the partition search space based on Quality
                                m_partitionSearchSize = std::max<double>( (1.0/16.0) , (m_quality / g_qFAST_THRESHOLD));
                            }
                            else
                            {
                                m_shakerRangeThreshold  = 255 * m_quality;     // lowers performance with incresing values
                                m_errorThreshold = 0;                         // Dont exit early 
                                m_partitionSearchSize   = 1.0;                 // use all partitions for best quality
                            }
                        }
#ifdef USE_DBGTRACE
                DbgTrace(("shakerRangeThreshold [%3.3f] errorThreshold [%3.3f] partitionSearchSize [%3.3f]",m_shakerRangeThreshold,m_errorThreshold,m_partitionSearchSize));
#endif
    };


    ~BC7BlockEncoder()
    {
#ifdef USE_DBGTRACE
                DbgTrace(("Smallest Error %f", (float)m_smallestError));
                DbgTrace(("Largest Error %f", (float)m_largestError));
#endif
    };

    // This routine compresses a block and returns the RMS error
    double CompressBlock(double in[MAX_SUBSET_SIZE][MAX_DIMENSION_BIG],
        unsigned char   out[COMPRESSED_BLOCK_SIZE]);

private:
    double quant_single_point_d(
        double data[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int numEntries, int index[MAX_ENTRIES],
        double out[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int epo_1[2][MAX_DIMENSION_BIG],
        int Mi_,                // last cluster
        int bits[3],            // including parity
        int type,
        int dimension
    );

    double ep_shaker_2_d(
        double data[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int numEntries,
        int index_[MAX_ENTRIES],
        double out[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int epo_code[2][MAX_DIMENSION_BIG],
        int size,
        int Mi_,             // last cluster
        int bits,            // total for all channels
                             // defined by total numbe of bits and dimensioin
        int dimension,
        double epo[2][MAX_DIMENSION_BIG]

    );

    double ep_shaker_d(
        double data[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int numEntries,
        int index_[MAX_ENTRIES],
        double out[MAX_ENTRIES][MAX_DIMENSION_BIG],
        int epo_code[2][MAX_DIMENSION_BIG],
        int Mi_,                // last cluster
        int bits[3],            // including parity
        CMP_qt type,
        int dimension
    );

    void    BlockSetup(unsigned int blockMode);
    void    EncodeSingleIndexBlock(unsigned int blockMode,
        unsigned int partition,
        unsigned int colour[MAX_SUBSETS][2],
                                   int   indices[MAX_SUBSETS][MAX_SUBSET_SIZE],
        unsigned char  block[COMPRESSED_BLOCK_SIZE]);

    // This routine compresses a block to any of the single index modes
    double CompressSingleIndexBlock(double in[MAX_SUBSET_SIZE][MAX_DIMENSION_BIG],
        unsigned char   out[COMPRESSED_BLOCK_SIZE],
        unsigned int  blockMode);

    void EncodeDualIndexBlock(unsigned int blockMode,
        unsigned int indexSelection,
        unsigned int componentRotation,
                              int endpoint[2][2][MAX_DIMENSION_BIG],
                              int indices[2][MAX_SUBSET_SIZE],
        unsigned char   out[COMPRESSED_BLOCK_SIZE]);

    // This routine compresses a block to any of the dual index modes
    double CompressDualIndexBlock(double in[MAX_SUBSET_SIZE][MAX_DIMENSION_BIG],
        unsigned char   out[COMPRESSED_BLOCK_SIZE],
        unsigned int  blockMode);

    // Bulky temporary data used during compression of a block
    int     m_storedIndices[MAX_PARTITIONS][MAX_SUBSETS][MAX_SUBSET_SIZE];
    double  m_storedError[MAX_PARTITIONS];
    int     m_sortedModes[MAX_PARTITIONS];

    // This stores the min and max for the components of the block, and the ranges
    double  m_blockMin[MAX_DIMENSION_BIG];
    double  m_blockMax[MAX_DIMENSION_BIG];
    double  m_blockRange[MAX_DIMENSION_BIG];
    double  m_blockMaxRange;

    // These are quality parameters used to select when to use the high precision quantizer
    // and shaker paths
    double m_quantizerRangeThreshold;
    double m_shakerRangeThreshold;
    double m_partitionSearchSize;
    
    // Global data setup at initialisation time
    double m_quality;
    double m_performance;
    double m_errorThreshold;
    unsigned int  m_validModeMask;
    bool   m_imageNeedsAlpha;
    bool   m_colourRestrict;
    bool   m_alphaRestrict;

    // Data for compressing a particular block mode
    unsigned int m_parityBits;
    unsigned int m_clusters[2];
    unsigned int m_componentBits[MAX_DIMENSION_BIG];

    // Error stats
    double m_smallestError;
    double m_largestError;

};


#endif