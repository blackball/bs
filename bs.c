/**
 * 
 * @blackball @date 3/13/2012
 */

#include "bs.h"
#include <time.h>
#include <stdlib.h>

#ifdef NDEBUG
#define bs_log
#else
#include <stdio.h>
#define bs_log fprintf
#endif

#ifndef BOOL
#define BOOL int
#define TRUE  1
#define FALSE 0
#endif

#ifndef ABS
#define ABS(v) ((v) > 0 ? (v) : -(v))
#endif

/* Duff's device to unroll loop  */
#define _unrollLoop(idx, count, _innerLoop)     \
        idx = (((count) - 1) & 7) - 7;          \
        switch (-idx) {                         \
                do {                            \
                case 0: _innerLoop(idx + 0);    \
                case 1: _innerLoop(idx + 1);    \
                case 2: _innerLoop(idx + 2);    \
                case 3: _innerLoop(idx + 3);    \
                case 4: _innerLoop(idx + 4);    \
                case 5: _innerLoop(idx + 5);    \
                case 6: _innerLoop(idx + 6);    \
                case 7: _innerLoop(idx + 7);    \
                        idx += 8;               \
                }while(idx < count);            \
        }


EXTERN_BEGIN

struct _bs_t {
        int sampleNum;
        int matchThresh;
        int matchNum;
        int updateFactor;
        int foregroundColor;
        int backgroundColor;
        int imageWidth;
        int widthStep;
        int imageHeight;
        int imageChannelNum;
        int colorMode; /* GRAY == 1, RGB == 3 */

        /* used for circlely random table */
        unsigned int randTableLen;
        unsigned int *randTable;
        unsigned int *randTable_curr;
        unsigned int *randTable_end;
  
        /* model and seg_map */
        unsigned char *bgModel;
        unsigned char *segMap;
};

static int _bs_createRandTable(bs_t *t) {
        unsigned int i;
        bs_t p = *t;

        srand((unsigned int)time(NULL));

        if ( p->randTable ) {
                bs_log(stderr, "Random table was already created !?\n");
                return 0;
        }

        p->randTable = (unsigned int*)malloc(p->randTableLen * sizeof(unsigned int));
        if (!p->randTable) {
                bs_log(stderr, "Error: Out of memory!\n");
                return -1;
        }

        /* randomly fill the table */
        for (i = 0; i < p->randTableLen; ++i)
                p->randTable[i] = rand();

        p->randTable_curr = (p->randTable);
        p->randTable_end  = (p->randTable + p->randTableLen);

        return 0;
}

static unsigned int _bs_getNextRandomNumber(bs_t t) {
        unsigned int curr = *((t)->randTable_curr);

        (t)->randTable_curr ++;
        if ( (t)->randTable_curr == (t)->randTable_end )
                (t)->randTable_curr = (t)->randTable;
  
        return curr;
}

/* create using default attributes */
int bs_create(bs_t *t) {
        bs_t p = NULL;
        /*  allocate memory, initialize them */
        *t = (bs_t)calloc( sizeof(struct _bs_t) , 1);
  
        if (!(*t)) {
                bs_log(stderr, "Error:Out of memory!\n");
                return -1;
        }
  
        p = *t;
        /* using default parameters */
        p->sampleNum = 20;
        p->matchThresh = 20;
        p->matchNum = 2;
        p->updateFactor = 16;
        p->foregroundColor = 0xFF; /* white */
        p->backgroundColor = 0x00; /* black */
        p->imageChannelNum = 1; /* gray-scale */
        p->colorMode = 1; /* means gray */
        p->imageWidth = 0;
        p->imageWidthStep = 0;
        p->imageHeight = 0;
        p->randTableLen = 0xFFFF;
        p->randTable = NULL;
        p->randTable_curr = NULL;
        p->randTable_end = NULL;
        p->bgModel = NULL;
        p->segMap = NULL;

        return 0;
}

static int _bs_getRandomNeighbour_8U_C1R(bs_t p, unsigned char *grayVal, unsigned char *imageData, int w, int ws, int h, int x, int y) {
        /* neighbour index */
        unsigned int nIdx;
        unsigned int stride;
        stride = ws;
        nIdx = x + w * y;

        /*
          5 6 7
          3 i 0
          3 2 1
        */
        switch(_bs_getNextRandomNumber(p) % 8) {
        case 0: 
                if (x < w - 1) 
                        nIdx += 1;
                break;
        case 1: 
                if (x < w - 1) 
                        nIdx += 1;
                if (y < h - 1) 
                        nIdx += stride;
                break;
        case 2: 
                if (y < h - 1) 
                        nIdx += stride;
                break;
        case 3: 
                if (x > 0) 
                        nIdx -= 1;
                if (y < h - 1) 
                        nIdx += stride;
                break;
        case 4: 
                if (x > 0) 
                        nIdx -= 1;
                break;
        case 5: 
                if (x > 0) 
                        nIdx -= 1;
                if (y > 0) 
                        nIdx -= stride;
                break;
        case 6: 
                if (y > 0) 
                        nIdx -= stride;
                break;
        case 7: 
                if (x < w - 1) 
                        nIdx += 1;
                if (y > 0) 
                        nIdx -= stride;
                break;
        default:
                ;
        }
  
        *grayVal = imageData[nIdx];
        return 0;
}

static int _bs_getRandomNeighbour_8U_C3R(bs_t p, unsigned char *v0, unsigned char *v1, unsigned char *v2, unsigned char *imageData, int w, int ws, int h, int x, int y) {
        /* neighbour index */
        unsigned int nIdx;
        unsigned int stride;
        stride = ws;
        nIdx  = 3*(x + w * y);

        /*
          5 6 7
          3 i 0
          3 2 1
        */
        switch(_bs_getNextRandomNumber(p) % 8) {
        case 0: 
                if (x < w - 1) 
                        nIdx+=3;
                break;
        case 1: 
                if (x < w - 1) 
                        nIdx+=3;
                if (y < h - 1) 
                        nIdx+=stride;
                break;
        case 2: 
                if (y < h - 1) 
                        nIdx+=stride;
                break;
        case 3: 
                if (x > 0) 
                        nIdx-=3;
                if (y < h - 1) 
                        nIdx+=stride;
                break;
        case 4: 
                if (x > 0) 
                        nIdx-=1;
                break;
        case 5: 
                if (x > 0) 
                        nIdx-=3;
                if (y > 0) 
                        nIdx-=stride;
                break;
        case 6: 
                if (y > 0) 
                        nIdx-=stride;
                break;
        case 7: 
                if (x < w - 1) 
                        nIdx+=3;
                if (y > 0) 
                        nIdx-=stride;
                break;
        default:
                ;
        }
  
        *v0 = imageData[nIdx + 0];
        *v1 = imageData[nIdx + 1];
        *v2 = imageData[nIdx + 2];
  
        return 0;
}

static int _bs_initModel_8U_C1R(bs_t p, unsigned char *imageData, int w, int ws, int h) {
        int x, y, k, step;
        unsigned char grayVal;
        unsigned char *pModel, *pData = imageData;
  
        if (!p->bgModel) {
                bs_log(stderr, "Error:background model not malloced!\n");
                return -1;
        }

        step = p->sampleNum; 
        pModel = p->bgModel;

        for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                        /* the first sample is the pixel itself */
                        *pModel++ = pData[x + y * ws];
                        for (k = 1; k < step; ++k) {
                                _bs_getRandomNeighbour_8U_C1R(p, &grayVal, pData, w, ws, h, x, y);
                                *pModel++ = grayVal;
                        }
                }
        }
        return 0;
}


static int _bs_initModel_8U_C3R(bs_t p, unsigned char *imageData, int w, int ws, int h) {
        int i, j, k, step, pos;
        unsigned char v0,v1,v2;
        unsigned char *pModel, *pData = imageData;
  
        if (!p->bgModel) {
                bs_log(stderr, "Error:background model not malloced!\n");
                return -1;
        }

        step = p->sampleNum;
        pModel = p->bgModel;
        for (i = 0; i < h; ++i) {
                for (j = 0; j < w; ++j) {
                        /* the first sample is the pixel itself */
                        pos = (j + i * ws) * 3;
                        *pModel++ = pData[pos + 0];
                        *pModel++ = pData[pos + 1];
                        *pModel++ = pData[pos + 1];
                        for (k = 1; k < step; ++k) {
                                _bs_getRandomNeighbour_8U_C3R(p, &v0, &v1, &v2, pData, w, ws, h, j, i);
                                *pModel++ = v0;
                                *pModel++ = v1;
                                *pModel++ = v2;
                        }
                }
        }
        return 0;
}

/* initialize background models */
int bs_initModel(bs_t *t, unsigned char *imageData,
                 int w, int ws, int h, int nChannels) {
        bs_t p;
        /* init random table && fill the background model */
        if (!t || !(*t) || !imageData) return -1;

        if ( w < 1 || h < 1) return -1; /* invalid size */

        if (_bs_createRandTable( t )) {
                bs_log(stderr, "Error:Create Random table failed!\n");
                return -1;
        }

        p = *t;
        p->imageWidth = w;
        p->imageWidthStep = ws;
        p->imageHeight = h;
        p->imageChannelNum = nChannels;
        if (nChannels != 1 && nChannels != 3) {
                bs_log(stderr, "Error:Unsupported image format!\n");
                return -1;
        }

        free ( p->bgModel ); /* could re-initialize */
        p->bgModel = (unsigned char*)calloc(p->sampleNum * nChannels * ws * h , sizeof(unsigned char));
        if (!p->bgModel) {
                bs_log(stderr, "Error:Out of memory!\n");
                return -1;
        }

        free ( p->segMap );
        p->segMap = (unsigned char*)calloc( ws * h, sizeof(unsigned char));
        if (!p->segMap) {
                bs_log(stderr, "Error:Out of memory!\n");
                return -1;
        }

        /* init background model depends on the channel number */
        switch( p->imageChannelNum ) {
        case 1:
                _bs_initModel_8U_C1R(p, imageData, w, ws, h);
                break;
        case 3:
                _bs_initModel_8U_C3R(p, imageData, w, ws, h);
                break;
        default:
                /* it shouldn't be here */
                ;
        }
        return 0;
}

static BOOL _bs_closeEnough_8U_C1R(unsigned char *pval, unsigned char *pixModel, int len, int matchTh, int matchNum) {
        int k, count;
        unsigned int delta;

        count = 0;
        for (k = 0; k < len; ++k) {
                delta = ABS(*pval - pixModel[k]);
                if (delta < matchTh) {
                        if (++count >= matchNum)
                                return TRUE;
                }
        }
        return FALSE;
}

static BOOL _bs_closeEnough_8U_C3R(unsigned char *pval, unsigned char *pixModel, int len, int matchTh, int matchNum) {
        int k, sampleLen, count, th;
        unsigned char v0, v1, v2;
        unsigned int delta;

        v0 = pval[0];
        v1 = pval[1];
        v2 = pval[2];

        sampleLen = len * 3;
        th = matchTh * 3;
        count = 0;
        for (k = 0; k < sampleLen; k += 3) {
                delta  = ABS(v0 - pixModel[k]);
                delta += ABS(v1 - pixModel[k+1]);
                delta += ABS(v2 - pixModel[k+2]);
                if (delta < th) {
                        if (++count >= matchNum)
                                return TRUE;
                }
        }
        return FALSE;
}

static int _bs_updatePixelModel_8u_C1R(bs_t p, unsigned char *pval, int w, int ws, int h, int x, int y)
{
        int nx, ny;
        unsigned int idx;
        /* update pixel model */
        unsigned int memberToReplace = _bs_getNextRandomNumber(p) % p->sampleNum;
        idx = (x + y * ws) * p->sampleNum + memberToReplace;
        p->bgModel[idx] = *pval;

        /* update corresponding pixel model */
        /* 5    6   7
           4  index 0
           3    2   1 */
        switch (_bs_getNextRandomNumber(p) % 8){
        case 0:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y;
                break;
        case 1:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 2:
                nx = x;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 3:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 4:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y;
                break;
        case 5:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        case 6:
                nx = x;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        case 7:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        }
  
        idx = (nx + ny * ws) * p->sampleNum + memberToReplace;
        p->bgModel[idx] = *pval;
  
        return 0;
}


static int _bs_updatePixelModel_8u_C3R(bs_t p, unsigned char *pval, int w, int ws, int h, int x, int y)
{
        int nx, ny, t;
        /* update pixel model */
        unsigned int memberToReplace = _bs_getNextRandomNumber(p) % p->sampleNum;

        t = 3 * p->sampleNum * (x + y * ws) + 3 * memberToReplace;
        p->bgModel[t + 0] = pval[0];
        p->bgModel[t + 1] = pval[1];
        p->bgModel[t + 2] = pval[2];
  
        /* update corresponding pixel model */
        /* 5    6   7
           4  index 0
           3    2   1 */
        switch (_bs_getNextRandomNumber(p) % 8){
        case 0:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y;
                break;
        case 1:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 2:
                nx = x;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 3:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y+1;
                if (ny == h)
                        ny--;
                break;
        case 4:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y;
                break;
        case 5:
                nx = x-1;
                if (nx == -1)
                        nx++;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        case 6:
                nx = x;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        case 7:
                nx = x+1;
                if (nx == w)
                        nx--;
                ny = y-1;
                if (ny == -1)
                        ny++;
                break;
        }
        t = 3 * p->sampleNum * (nx + ny * ws) + 3 * memberToReplace;;
        p->bgModel[t + 0] = pval[0];
        p->bgModel[t + 1] = pval[1];
        p->bgModel[t + 2] = pval[2];
  
        return 0;
}

/* perform background sbstraction and update model */
int bs_update(const bs_t *t, unsigned char *frame, unsigned char *segmap) {
        int w, h, x, y, step, matchTh, matchNum;
        int isUpdateModel, updateFactor;
        unsigned char bgColor, foreColor;
        unsigned char *sm, *pFrame, *pModel;
        bs_t p = *t;
  
        if ( !t || !p ) return -1;

        if (!p->segMap || !p->bgModel) return -1;

        sm = p->segMap;
        pFrame = frame;
        pModel = p->bgModel;
        step = p->sampleNum;
        matchTh = p->matchThresh;
        matchNum = p->matchNum;
        bgColor = p->backgroundColor;
        foreColor = p->foregroundColor;
        updateFactor = p->updateFactor;
        w = p->imageWidth;
        ws = p->imageWidthStep;
        h = p->imageHeight;

        switch(p->imageChannelNum) {
        case 1: 
                for (y = 0; y < h; ++y) {
                        for (x = 0; x < w; ++x) {
                                if (_bs_closeEnough_8U_C1R(pFrame, pModel, step, matchTh, matchNum)) {
                                        *sm = bgColor;
                                        isUpdateModel = ((_bs_getNextRandomNumber(p) % (updateFactor)) == (updateFactor - 1));
                                        if (isUpdateModel) {
                                                _bs_updatePixelModel_8u_C1R(p, pFrame, w, ws, h, x, y);
                                        }
                                } else {
                                        *sm = foreColor;
                                }
                                pModel += step;
                                ++ pFrame;
                                ++ sm;
                        }
                }
        
                break;

        case 3:
                for (y = 0; y < h; ++y) {
                        for (x = 0; x < w; ++x) {
                                *sm = foreColor;

                                if (_bs_closeEnough_8U_C3R(pFrame, pModel, step, matchTh, matchNum)) {
                                        *sm = bgColor;

                                        isUpdateModel = ((_bs_getNextRandomNumber(p) % (updateFactor)) == (updateFactor - 1));
                                        if (isUpdateModel) {
                                                _bs_updatePixelModel_8u_C3R(p, pFrame, w, ws, h, x, y);
                                        }
                                }

                                pModel += 3*step;
                                pFrame += 3;
                                sm++;
                        }
                }      
                break;

        default:
                /* it shouldn't be here */
                ;
        }
        /* shouldn't be freed outside ;( */
        segmap = p->segMap;
  
        return 0;
}

/* destroy */
int bs_destroy(bs_t *t) {
        if (!t || !(*t)) return 0;
        free ((*t)->randTable);
        free ((*t)->bgModel);
        free ((*t)->segMap);
        free(*t);
        *t = NULL;
        return 0;
}

/********geters && setter***********/

int bs_getSampleNum(const bs_t p) {
        return p->sampleNum;
}

int bs_getMatchNum(const bs_t p) {
        return p->matchNum;
}

int bs_getMatchThreshold(const bs_t p) {
        return p->matchThresh;
}

int bs_getUpdateFactor(const bs_t p) {
        return p->updateFactor;
}

int bs_getColorMode(const bs_t p) {
        return p->colorMode;
}

unsigned int bs_getRandTableLen(const bs_t p) {
        return p->randTableLen;
}

/* be careful with this */
unsigned char* bs_getModelPtr(const bs_t p) {
        return p->bgModel;
}

unsigned char* bs_getSegMapPtr(const bs_t p) {
        return p->segMap;
}

unsigned char bs_getBackgroundColor(const bs_t p) {
        return p->backgroundColor;
}

unsigned char bs_getForegroundColor(const bs_t p) {
        return p->foregroundColor;
}

int bs_getImageChannelNum(const bs_t p) {
        return p->imageChannelNum;
}

void bs_setForegroundColor(const bs_t p, unsigned char c) {
        p->foregroundColor = c;
}

void bs_setBackgroundColor(const bs_t p, unsigned char c) {
        p->backgroundColor = c;
}

void bs_setSampleNum(const bs_t p, int n) {
        p->sampleNum = n;
}

void bs_setMatchNum(const bs_t p, int n) {
        p->matchNum = n;
}

void bs_setMatchThreshold(const bs_t p, int th) {
        p->matchThresh = th;
}

void bs_setUpdateFactor(const bs_t p, int f) {
        p->updateFactor = f;
}

void bs_setRandTableLen(const bs_t p, unsigned int len) {
        p->randTableLen = len;
}

/* It's dangerous to provide those two APIs, but
 * the whole thing was created in C, you can't
 * avoid it, 'C trust programmers'.
 */
void bs_setModelPtr(const bs_t p, unsigned char *newModel) {
        free (p->bgModel);
        p->bgModel = newModel;
}

void bs_setSegMapPtr(const bs_t p, unsigned char *newSegMap) {
        free(p->segMap);
        p->segMap = newSegMap;
}

EXTERN_END
