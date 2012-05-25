/**
 * @file   vibecpp.h
 * @author blackball <bugway@gmail.com>
 * @date   Wed Feb 29 23:32:19 2012
 * 
 * @brief  C++ version of VIBE.
 * @note A pixel-patch ( 2x2, 3x3, ... ) filter approach
 * will be presented in next version.
 */

#ifndef BGOOTH_CPP_H
#define BGOOTH_CPP_H

#include <time.h>
#include <stdlib.h>
#include <assert.h>

#define BGOOTH_COLOR_BACKGROUND 0x00
#define BGOOTH_COLOR_FOREGROUND 0xFF

#define _bgooth_unrollLoop(idx, count, _innerLoop)      \
        idx = (((count) - 1) & 7) - 7;                  \
        switch (-idx) {                                 \
                do {                                    \
                case 0: _innerLoop(idx + 0);            \
                case 1: _innerLoop(idx + 1);            \
                case 2: _innerLoop(idx + 2);            \
                case 3: _innerLoop(idx + 3);            \
                case 4: _innerLoop(idx + 4);            \
                case 5: _innerLoop(idx + 5);            \
                case 6: _innerLoop(idx + 6);            \
                case 7: _innerLoop(idx + 7);            \
                        idx += 8;                       \
                }while(idx < count);                    \
        }


class BGooth {
public:
        BGooth(
                int nSamples = 20, /* number of random samples for every pixel */
                int matchingTh = 20, /* value threshold for matching two pixel values */
                int matchingNum = 2, /* threshold for judge if a pixel is foreground */
                int updateFactor = 16 /* @TODO I don't make this clear too */
                ):
                m_nChannels(1), /* gray-scale is the default format */
                m_numSamples(nSamples),
                m_matchingThreshold(matchingTh),
                m_matchingNumber(matchingNum),
                m_updateFactor(updateFactor),
                m_width(0),
                m_widthStep(0),
                m_height(0),
                m_bgmodel(NULL),
                m_randTable(NULL){
                // pass
        }

        int ModelInit(const unsigned char *imageData,
                      int imgW,
                      int imgWS,
                      int imgH,
                      int nChannels) {
                /* @TODO safety check */
                switch(nChannels) {
                case 1:
                        /* gray scale */
                        return ModelInit_C1R(imageData, imgW, imgWS, imgH);
                case 3:
                        /* three channels color image */
                        return ModelInit_C3R(imageData, imgW, imgWS, imgH);        
                default:
                        // fprintf(stderr, "Error: unsupport image format!\n");
                        return -1;
                }
        }

        int ModelUpdate(const unsigned char *imageData,
                        unsigned char *segmentationMap){
                // pass
                switch(this->m_nChannels) {
                case 1:
                        return ModelUpdate_C1R( imageData, segmentationMap);
                case 3:
                        return ModelUpdate_C3R( imageData, segmentationMap);
                default:
                        return -1;
                }
        }
  
        // also, you could call methods directly
       

        inline int ModelInit_C1R(const unsigned char *imageData,
                                 int width,
                                 int widthStep,
                                 int height) {
			

#define __lazy(type, nChannels)                                         \
                this->m_nChannels = nChannels;                          \
                /* 1. init random table */                              \
                this->initRandTable(0xFFFF);                            \
                /* init bg model */                                     \
                assert( imageData );                                    \
                assert(width > 0 && height > 0);                        \
                this->m_width = width;                                  \
                this->m_height = height;                                \
                int pstep = this->m_numSamples;                         \
                this->m_bgmodel =                                       \
                        (unsigned char*)calloc(widthStep*height*pstep*nChannels, \
                                               sizeof(unsigned char));  \
                assert( this->m_bgmodel );                              \
                unsigned char *ptr = this->m_bgmodel;                   \
                for (int h = 0; h < height; ++h) {                      \
                        for (int w = 0; w < width; ++w) {               \
                                this->getRandomNeighborPixels_##type(imageData, \
                                                                     width, \
                                                                     widthStep, \
                                                                     height, \
                                                                     w,h, \
                                                                     ptr, pstep); \
                                ptr += pstep*nChannels;                 \
                        }                                               \
                }                                                           
    
                __lazy( C1R, 1 );
        
                return 0;
        }
  
        inline int ModelInit_C3R(const unsigned char *imageData,
                                 int width,
                                 int height) {
                __lazy( C3R, 3 );
    
                return 0;

#undef __lazy
        }
  
        inline int ModelUpdate_C1R(const unsigned char *imageData,
                                   unsigned char *segMap){
#define __lazy(type, nChannels)                                         \
                unsigned char                                           \
                        *bgPtr = this->m_bgmodel,                       \
                        *segPtr = segMap;                               \
                const unsigned char *imgPtr = imageData;                \
                int                                                     \
                        matchingNum = this->m_matchingNumber,           \
                        matchingTh = this->m_matchingThreshold,         \
                        pstep = this->m_numSamples,                     \
                        imgW = this->m_width,                           \
                        imgWS = this->m_widthStep,                      \
                        imgH = this->m_height,                          \
                        updateFactor = this->m_updateFactor;            \
                /* suppose image data was aligned properly */           \
                for (int h = 0; h < imgH; ++h)                          \
                        for (int w = 0; w < imgW; ++w) {                \
                                if (this->pixelCloseEnough_##type(imgPtr,bgPtr)) { \
                                        *segPtr = BGOOTH_COLOR_BACKGROUND; \
                                        /* update frequency */          \
                                        if (this->crand() % updateFactor == 0) { \
                                                this->updateModelWidthSample_##type(imgPtr,bgPtr, \
                                                                                    w, imgWS, h); \
                                        }                               \
                                } else {                                \
                                        /* pixel in the foreground */   \
                                        *segPtr = BGOOTH_COLOR_FOREGROUND; \
                                }                                       \
                                ++ segPtr;                              \
                                bgPtr += pstep*nChannels;               \
                                imgPtr += nChannels;                    \
                        }

                __lazy( C1R, 1 );
    
                return 0;
        }

        inline int ModelUpdate_C3R(const unsigned char *imageData,
                                   unsigned char *segMap) {
                __lazy( C3R, 3 );
 
#undef __lazy
                return 0;
        }
  
        ~BGooth(){
                free (m_bgmodel);
                free (m_randTable);
                m_bgmodel = NULL;
                m_randTable = NULL;
        }

private:
        inline void initRandTable(unsigned int len){
                /* RAND_TAB_LEN + 1 maybe dangerous */
                srand((unsigned int)time(NULL));
                                
                m_randTable = (unsigned int*)malloc((len + 1) * sizeof(unsigned int));
                assert(m_randTable);
                
                for (unsigned int i = 0; i <= len; ++i)
                        m_randTable[i] = rand();

                /* point to the first and the last elements */
                m_currTabPos = m_randTable;
                m_lastTabPos = m_randTable + len;
        }
  
        /* circular rand */
        inline unsigned int crand() {
                unsigned int t = *m_currTabPos;
                if (m_currTabPos != m_lastTabPos) {
                        ++m_currTabPos;
                        return t;
                }    
                m_currTabPos = m_randTable;
                return t;
        }

        inline void getRandomNeighborPixels_C1R(const unsigned char *imageData,
                                                int width, int widthStep,
                                                int height,
                                                int w, int h,
                                                unsigned char *ptr,
                                                int pstep) {

                const unsigned char *imgPtr = imageData + h * widthStep + w;
    
                /* fill the pixel model */
                /*
                  5 6 7
                  4 i 0
                  3 2 1
                */
                unsigned char neighbors[8] = {*imgPtr,*imgPtr,*imgPtr,*imgPtr,
                                              *imgPtr,*imgPtr,*imgPtr,*imgPtr};
                /* boundw = 1 or 2 means at the left or right boundary 
                 * boundh = 1 or 2 means at the up or bottom boundary
                 */
                unsigned boundw = (w == 0) + ((w == width)  << 1);
                unsigned boundh = (h == 0) + ((h == height) << 1);

#define _at(x,y) (*(imgPtr + y*widthStep+ x))

                switch(boundw + (boundh << 3)) {
                case 0: /* not at the boundary */
                        neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        neighbors[1] = _at( 1,  1);
                        break;
        
                case 1:
                        //neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        neighbors[7] = _at( 1, -1);
                        //neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        //neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        neighbors[1] = _at( 1,  1);
                        break;
        
                case 2:
                        neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        //neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        //neighbors[0] = _at( 1,  0);
                        neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        //neighbors[1] = _at( 1,  1);
                        break;
        
                case 8:
                        //neighbors[5] = _at(-1, -1);
                        //neighbors[6] = _at( 0, -1);
                        //neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        neighbors[1] = _at( 1,  1);
                        break;
        
                case 9:
                        //neighbors[5] = _at(-1, -1);
                        //neighbors[6] = _at( 0, -1);
                        //neighbors[7] = _at( 1, -1);
                        //neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        //neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        neighbors[1] = _at( 1,  1);
                        break;
        
                case 10:
                        // neighbors[5] = _at(-1, -1);
                        //neighbors[6] = _at( 0, -1);
                        //neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        //neighbors[0] = _at( 1,  0);
                        neighbors[3] = _at(-1,  1);
                        neighbors[2] = _at( 0,  1);
                        //neighbors[1] = _at( 1,  1);
                        break;
            
                case 16:
                        neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        //neighbors[3] = _at(-1,  1);
                        //neighbors[2] = _at( 0,  1);
                        //neighbors[1] = _at( 1,  1);
                        break;
        
                case 17:
                        //neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        neighbors[7] = _at( 1, -1);
                        //neighbors[4] = _at(-1,  0);
                        neighbors[0] = _at( 1,  0);
                        //neighbors[3] = _at(-1,  1);
                        //neighbors[2] = _at( 0,  1);
                        //neighbors[1] = _at( 1,  1);
                        break;
        
                case 18:
                        neighbors[5] = _at(-1, -1);
                        neighbors[6] = _at( 0, -1);
                        //neighbors[7] = _at( 1, -1);
                        neighbors[4] = _at(-1,  0);
                        //neighbors[0] = _at( 1,  0);
                        //neighbors[3] = _at(-1,  1);
                        //neighbors[2] = _at( 0,  1);
                        //neighbors[1] = _at( 1,  1);
                default:
                        // It's impossible to get here
                        break;
                }

#undef _at

                unsigned char *bgPtr = ptr;
                *bgPtr++ = *imgPtr;

                for (int i = pstep - 1; i; --i) {
                        *bgPtr++ = neighbors[ this->crand() & 7 ]; // N % 8 == N & ( 8-1 ) 
                }

                return ;
        }

        inline void getRandomNeighborPixels_C3R(const unsigned char *imageData,
                                                int width, int widthStep,
                                                int height,
                                                int w, int h,
                                                unsigned char *ptr,
                                                int pstep) {

                const unsigned char *imgPtr = imageData + 3*(h * widthStep + w);
    
                /* fill the pixel model */
                /*
                  5 6 7
                  4 i 0
                  3 2 1
                */
                unsigned char neighbors[8][3] = {{imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]},
                                                 {imgPtr[0], imgPtr[1], imgPtr[2]}};
                /* boundw = 1 or 2 means at the left or right boundary 
                 * boundh = 1 or 2 means at the up or bottom boundary
                 */
                unsigned boundw = (w == 0) + ((w == width)  << 1);
                unsigned boundh = (h == 0) + ((h == height) << 1);

#define _at(x,y,i) (*(imgPtr + 3*(widthStep*y+x) + i))
    
#define _asspixel(pos, x, y)                    \
                neighbors[pos][0] = _at(x,y,0); \
                neighbors[pos][1] = _at(x,y,1); \
                neighbors[pos][2] = _at(x,y,2)
    
                switch(boundw + (boundh << 3)) {
                case 0: /* not at the boundary */
                        _asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        _asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        _asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        _asspixel(1,  1,  1);
                        break;
         
                case 1:
                        //_asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        _asspixel(7,  1, -1);
                        //_asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        //_asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        _asspixel(1,  1,  1);
                        break;
        
                case 2:
                        _asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        //_asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        //_asspixel(0,  1,  0);
                        _asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        //_asspixel(1,  1,  1);
                        break;
 
                case 8:
                        //_asspixel(5, -1, -1);
                        //_asspixel(6,  0, -1);
                        //_asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        _asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        _asspixel(1,  1,  1);
                        break;
 
                case 9:
                        //_asspixel(5, -1, -1);
                        //_asspixel(6,  0, -1);
                        //_asspixel(7,  1, -1);
                        //_asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        //_asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        _asspixel(1,  1,  1);
                        break;
        
                case 10:
                        //_asspixel(5, -1, -1);
                        //_asspixel(6,  0, -1);
                        //_asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        //_asspixel(0,  1,  0);
                        _asspixel(3, -1,  1);
                        _asspixel(2,  0,  1);
                        //_asspixel(1,  1,  1);
                        break;
        
                case 16:
        
                        _asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        _asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        //_asspixel(3, -1,  1);
                        //_asspixel(2,  0,  1);
                        //_asspixel(1,  1,  1);
                        break;

                case 17:
                        //_asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        _asspixel(7,  1, -1);
                        //_asspixel(4, -1,  0);
                        _asspixel(0,  1,  0);
                        //_asspixel(3, -1,  1);
                        //_asspixel(2,  0,  1);
                        //_asspixel(1,  1,  1);
                        break;

                case 18:
        
                        _asspixel(5, -1, -1);
                        _asspixel(6,  0, -1);
                        //_asspixel(7,  1, -1);
                        _asspixel(4, -1,  0);
                        //_asspixel(0,  1,  0);
                        //_asspixel(3, -1,  1);
                        //_asspixel(2,  0,  1);
                        //_asspixel(1,  1,  1);
                        break;

                default:
                        // It's impossible to get here
                        break;
                }

#undef _asspixel
#undef _at

                unsigned char *bgPtr = ptr;
                unsigned idx;
                *bgPtr ++ = imgPtr[0];
                *bgPtr ++ = imgPtr[1];
                *bgPtr ++ = imgPtr[2];
                for (int i = pstep - 1; i; --i) {
                        idx = this->crand() & 7; // N % 8 == N & ( 8-1 ) 
                        bgPtr[0] = neighbors[ idx ][0];
                        bgPtr[1] = neighbors[ idx ][1];
                        bgPtr[2] = neighbors[ idx ][2];
                        bgPtr += 3;
                }

                return ;
        }

  
        inline int pixelCloseEnough_C1R(const unsigned char *pixelPtr,
                                        unsigned char *pixelModel){
    
                int
                        count = 0, /* count the matched pixels */
                        pixelVal = *pixelPtr,
                        nSamples = this->m_numSamples,
                        matchingNum = this->m_matchingNumber,
                        matchingTh = this->m_matchingThreshold;
    
                unsigned char *modelPtr = pixelModel;
    
                /* calculate distances to pixel model */
#define _abs(v) ((v) < 0 ? -(v) : (v))

                for (int k = 0; k < nSamples; ++k) {
                        if ( _abs( pixelVal - modelPtr[k] ) < matchingTh ) {
                                if ( ++ count >= matchingNum ) 
                                        return 1; 
                        }
                }
    
#undef _abs

                return 0;
        }

        inline int pixelCloseEnough_C3R(const unsigned char *pixelPtr,
                                        unsigned char *pixelModel) {
    
                int
                        count = 0,
                        val0 = pixelPtr[0],
                        val1 = pixelPtr[1],
                        val2 = pixelPtr[2],
                        nSamples = this->m_numSamples,
                        matchingNum = this->m_matchingNumber,
                        matchingTh = this->m_matchingThreshold;

                unsigned char *modelPtr = pixelModel;
        
#define _abs(v) ((v) < 0 ? -(v) : (v))

#define _bgooth_loop(idx)                               \
                delta  = _abs(val0 - modelPtr[idx]);    \
                delta += _abs(val1 - modelPtr[idx+1]);  \
                delta += _abs(val2 - modelPtr[idx+2]);  \
                if ( delta < 3 * matchingTh ) {         \
                        if ( ++count  >= matchingNum )  \
                                return 1;               \
                }

                /* optimization using Duff's device */
                int delta = 0, i = 0;
                _bgooth_unrollLoop(i, nSamples, _bgooth_loop);
#undef _bgooth_loop
    
                /* The original code is below */
                /*
                  for (int i = 0; i < nSamples; ++i) {
                  delta  = _abs(val0 - modelPtr[i]);
                  delta += _abs(val1 - modelPtr[i+1]);
                  delta += _abs(val2 - modelPtr[i+2]);
                  if ( delta < 3 * matchingTh ) {
                  if ( ++count  >= matchingNum )
                  return 1;
                  }
                  }
                */
    
#undef _abs
                return 0;
        }

        inline  void updateModelWidthSample_C1R(const unsigned char *pixelPtr,
                                                unsigned char *modelPtr,
                                                int x, int y) {
                int
                        imgW = this->m_width,
                        imgH = this->m_height,
                        nSamples = this->m_numSamples;

                int randReplace = this->crand() % nSamples;
                modelPtr[randReplace] = *pixelPtr;

                /* @TODO need more clean codes */
    
                /* update corresponding pixel model */
                /* 5   6    7
                   4  index 0
                   3   2    1
                */    
                int nx = 0, ny = 0; /* neighbour's position */

                /* this routine is a bit slow, but more clean. */
                switch ( this->crand() & 7 ){
                case 0:
                        ++nx;
                        break;
                case 1:
                        ++nx; ++ny;
                        break;
                case 2:
                        ++ny;
                        break;
                case 3:
                        --nx; ++ny;
                        break;
                case 4:
                        --nx;
                        break;
                case 5:
                        --nx; --ny;
                        break;
                case 6:
                        --ny;
                        break;
                case 7:
                        ++nx; --ny;
                        break;
                default:
                        /* impossible to get here */
                        break;
                }

                if (nx + x == -1) nx = 0;
                else if (nx + x == imgW) --nx;

                if (ny + y == -1) ny = 0;
                else if (ny + y == imgH) --ny;
    
                modelPtr[(nx + imgWS * ny) * nSamples + randReplace] = *pixelPtr;

                return ;
        }
  
        inline void updateModelWidthSample_C3R(const unsigned char *pixelPtr,
                                               unsigned char *modelPtr,
                                               int x, int y) {
                int
                        imgW = this->m_width,
                        imgH = this->m_height,
                        nSamples = this->m_numSamples,
                        v0 = pixelPtr[0],
                        v1 = pixelPtr[1],
                        v2 = pixelPtr[2];

                int randReplace = this->crand() % nSamples;

                modelPtr[randReplace] = v0;
                modelPtr[randReplace + 1] = v1;
                modelPtr[randReplace + 2] = v2;
                /* @TODO need more clean codes */
    
                /* update corresponding pixel model */
                /* 5   6    7
                   4  index 0
                   3   2    1
                */    
                int nx = 0, ny = 0; /* neighbour's position */

                /* this routine is a bit slow, but more clean. */
                switch ( this->crand() & 7 ){
                case 0:
                        ++nx;
                        break;
                case 1:
                        ++nx; ++ny;
                        break;
                case 2:
                        ++ny;
                        break;
                case 3:
                        --nx; ++ny;
                        break;
                case 4:
                        --nx;
                        break;
                case 5:
                        --nx; --ny;
                        break;
                case 6:
                        --ny;
                        break;
                case 7:
                        ++nx; --ny;
                        break;
                default:
                        /* impossible to get here */
                        break;
                }

                if (nx + x == -1) nx = 0;
                else if (nx + x == imgW) --nx;

                if (ny + y == -1) ny = 0;
                else if (ny + y == imgH) --ny;
    
                unsigned int pos = 3*(nSamples * (nx + imgWS * ny) + randReplace);
                modelPtr[pos] = v0;
                modelPtr[pos + 1] = v1;
                modelPtr[pos + 2] = v2;

                return ;
        }

  
private:
        int m_nChannels;
        int m_numSamples;
        int m_matchingThreshold;
        int m_matchingNumber;
        int m_updateFactor;

        /* pixel-wise background model */
        int m_width, m_widthStep, m_height;
        unsigned char *m_bgmodel;

        /* circle random table */
        unsigned int *m_currTabPos;
        unsigned int *m_lastTabPos;
        unsigned int *m_randTable;
};

#undef _bgooth_unrollLoop

#endif
