/**
 * 
 * @blackball @date 3/13/2012
 */

#include "bs.h"
#include <time.h>
#include <stdlib.h>

#ifdef BS_DEBUG
#include <stdio.h>
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
  idx = (((count) - 1) & 7) - 7;                \
  switch (-idx) {                               \
    do {                                        \
      case 0: _innerLoop(idx + 0);              \
      case 1: _innerLoop(idx + 1);              \
      case 2: _innerLoop(idx + 2);              \
      case 3: _innerLoop(idx + 3);              \
      case 4: _innerLoop(idx + 4);              \
      case 5: _innerLoop(idx + 5);              \
      case 6: _innerLoop(idx + 6);              \
      case 7: _innerLoop(idx + 7);              \
        idx += 8;                               \
    }while(idx < count);                        \
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
  
  if ( !p->randTable ) {
#ifdef BS_DEBUG
    fprintf(stderr, "Random table was already created !?\n");
#endif
    return 0;
  }

  p->randTable = (int*)malloc(p->randTableLen);
  if (!p->randTable) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error: Out of memory!\n");
#endif
    return -1;
  }

  /* randomly fill the table */
  srand((unsigned int)time(NULL));
  for (i = 0; i < p->randTableLen; ++i)
    p->randTable[i] =rand();

  p->randTable_curr = &(p->randTable);
  p->randTable_end  = &(p->randTable[p->randTable - 1]);

  return 0;
}

static unsigned int _bs_getNextRandomNumber(bs_t *t) {
  unsigned int curr = *((*t)->randTable_curr);  
  if ( curr == (*t)->randTable_end )
    (*t)->randTable_curr = (*t)->randTable;
  
  return curr;
}

/* create using default attributes */
int bs_create(bs_t *t) {
  bs_t p = NULL;
  /*  allocate memory, initialize them */
  *t = (bs_t)calloc( sizeof(_bs_t) , 1);
  
  if (!(*t)) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:Out of memory!\n");
#endif
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
  p->imageHeight = 0;
  p->randTableLen = 0xFFFF;
  p->randTable = NULL;
  p->randTable_curr = NULL;
  p->randTable_end = NULL;
  p->bgModel = NULL;
  p->segMap = NULL;

  return 0;
}

static int _bs_getRandomNeighbour_8U_C1R(bs_t p, unsigned char *pix, unsigned char *imageData, int w, int h, int x, int y) {
  /* neighbour index */
  unsigned int nIdx;
  unsigned int stride;
  stride = w;
  nIdx =  = x + stride * y;

  /*
    5 6 7
    3 i 0
    3 2 1
  */
  switch(_bs_getNextRandomNumber(p) % 8) {
    case 0: 
      if (x < w - 1) 
        nIdx+=1;
      break;
    case 1: 
      if (x < w - 1) 
        nIdx+=1;
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 2: 
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 3: 
      if (x > 0) 
        nIdx-=1;
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 4: 
      if (x > 0) 
        nIdx-=1;
      break;
    case 5: 
      if (x > 0) 
        nIdx-=1;
      if (y > 0) 
        nIdx-=stride;
      break;
    case 6: 
      if (y > 0) 
        nIdx-=stride;
      break;
    case 7: 
      if (x < w - 1) 
        nIdx+=1;
      if (y > 0) 
        nIdx-=stride;
      break;
    default:
      ;
  }
  
  *pix = imageData[nIdx];
  return 0;
}

static int _bs_getRandomNeighbour_8U_C3R(bs_t p, unsigned char *pix, unsigned char *imageData, int w, int h, int x, int y) {
  /* neighbour index */
  unsigned int nIdx;
  unsigned int stride;
  stride = w * 3;
  nIdx  = x + stride * y;

  /*
    5 6 7
    3 i 0
    3 2 1
  */
  switch(_bs_getNextRandomNumber(p) % 8) {
    case 0: 
      if (x < w - 1) 
        nIdx+=1;
      break;
    case 1: 
      if (x < w - 1) 
        nIdx+=1;
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 2: 
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 3: 
      if (x > 0) 
        nIdx-=1;
      if (y < h - 1) 
        nIdx+=stride;
      break;
    case 4: 
      if (x > 0) 
        nIdx-=1;
      break;
    case 5: 
      if (x > 0) 
        nIdx-=1;
      if (y > 0) 
        nIdx-=stride;
      break;
    case 6: 
      if (y > 0) 
        nIdx-=stride;
      break;
    case 7: 
      if (x < w - 1) 
        nIdx+=1;
      if (y > 0) 
        nIdx-=stride;
      break;
    default:
      ;
  }
  
  *pix++ = imageData[nIdx + 0];
  *pix++ = imageData[nIdx + 1];
  *pix++ = imageData[nIdx + 2];
  
  return 0;
}

static int _bs_initModel_8U_C1R(bs_t p, unsigned char *imageData, int w, int h) {
  int i, j, k, step;
  unsigned char *pModel, *pData = imageData;
  
  if (!p->bgModel) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:background model not malloced!\n");
#endif
    return -1;
  }

  step = p->sampleNum; 
  pModel = p->bgModel;
  for (i = 0; i < h; ++i) {
    for (j = 0; j < width; ++j) {
      /* the first sample is the pixel itself */
      *pModel++ = pData[j + i * h];
      for (k = 1; k < step; ++j)
        _bs_getRandomNeighbour_8U_C1R(p, pModel, pData, w, h, j, i);
    }
  }

  return 0;
}


static int _bs_initModel_8U_C3R(bs_t p, unsigned char *imageData, int w, int h) {
  int i, j, k, step;
  unsigned char *pModel, *pData = imageData;
  
  if (!p->bgModel) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:background model not malloced!\n");
#endif
    return -1;
  }

  step = p->sampleNum;
  pModel = p->bgModel;
  for (i = 0; i < h; ++i) {
    for (j = 0; j < width; ++j) {
      /* the first sample is the pixel itself */
      *pModel++ = pData[j + i * h];
      for (k = 1; k < step; ++j)
        _bs_getRandomNeighbour_8U_C3R(p, pModel, pData, w, h, j, i);
    }
  }

  return 0;
}


/* initialize background models */
int bs_initModel(bs_t *t, unsigned char *imageData,
                 int w, int h, int nChannels) {
  bs_t p;
  /* init random table && fill the background model */
  if (!t || !(*t) || !imageData) return -1;

  if ( w < 1 || h < 1) return -1; /* invalid size */

  if (!_bs_createRandTable( t )) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:Create Random table failed!\n");
#endif
    return -1;
  }

  p = *t;
  p->imageWidth = w;
  p->imageHeight = h;
  p->imageChannelNum = nChannels;
  if (nChannels != 1 && nChannels != 3) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:Unsupported image format!\n");
#endif
    return -1;
  }

  free ( p->bgModel ); /* could re-initialize */
  p->bgModel = (unsigned char*)calloc(p->sampleNum * nChannels * w * h , sizeof(unsigned char));
  if (!b->bgModel) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:Out of memory!\n");
#endif
    return -1;
  }

  free ( p->segMap );
  p->segMap = (unsigned char*)calloc( w * h, sizeof(unsigned char));
  if (!b->segMap) {
#ifdef BS_DEBUG
    fprintf(stderr, "Error:Out of memory!\n");
#endif
    return -1;
  }

  /* init background model depends on the channel number */
  switch( p->imageChannelNum ) {
    case 1:
      _bs_initModel_8U_C1R(p, imageData, w, h);
      break;
    case 3:
      _bs_initModel_8U_C3R(p, imageData, w, h);
      break;
    default:
      /* it shouldn't be here */
      ;
  }
  return 0;
}

static BOOL _bs_closeEnough_8U_C1R(unsigned char *pval, unsigned char *pixModel, int len, int matchTh, int matchNum) {
  int k, count;
  unsigned char delta;

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
  int k, count, th;
  unsigned char delta;
  
  th = matchTh * 3;
  count = 0;
  for (k = 0; k < len; ++k) {
    delta = ABS(*pval - pixModel[k]);
    delta += ABS(*(pval+1) - pixModel[k+1]);
    delta += ABS(*(pval+2) - pixModel[k+2]);
    if (delta < th) {
      if (++count >= matchNum)
        return TRUE;
    }
  }
  return FALSE;
}

static int _bs_updatePixelModel_8u_C1R(bs_t p, unsigned char *pval, int w, int h, int x, int y)
{
  int nx, ny;
  /* update pixel model */
  unsigned int memberToReplace = _bs_getNextRandomNumber(p) % p->sampeNum;
  p->bgModel[x + y * width + memberToReplace] = *pval;

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
  
  p->bgModel[nx + ny * width + memberToReplace] = *pval;
  
  return 0;
}


static int _bs_updatePixelModel_8u_C3R(bs_t p, unsigned char *pval, int w, int h, int x, int y)
{
  int nx, ny, t;
  /* update pixel model */
  unsigned int memberToReplace = _bs_getNextRandomNumber(p) % p->sampeNum;
  t = 3*(x + y * width + memberToReplace);
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
  t = 3*(nx + ny * width + memberToReplace);
  p->bgModel[t + 0] = pval[0];
  p->bgModel[t + 1] = pval[1];
  p->bgModel[t + 2] = pval[2];
  
  return 0;
}

/* perform background sbstraction and update model */
int bs_update(const bs_t *t, unsigned char *frame, unsigned char *segmap) {
  int w, h, i, j, step, matchTh, matchNum;
  unsigned int isUpdateModel, updateFactor;
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
  matchNum = p->matchNumber;
  bgColor = p->backgroundColor;
  foreColor = p->foregroundColor;
  updateFactor = p->updateFactor;
  
  switch(p->imageChannelNum) {
    case 1: {
      for (i = 0; i < h; ++i) {
        for (j = 0; j < w; ++j) {
          if (_bs_closeEnough_8U_C1R(pFrame, pModel, step, matchTh, matchNum)) {
            *sm = bgColor;
            isUpdateModel = (_bs_getNextRandomNumber(p) % updateFactor);
            if (isUpdateModel) {
              _bs_updatePixelModel_8U_C1R(p, *pFrame, w, h, j,i);
            }
          } else {
            *sm = foreColor;
          }
          pModel += step;
          ++ pFrame;
          ++ sm;
        }
      }
    }
      break;
    case 3:{
      for (i = 0; i < h; ++i) {
        for (j = 0; j < w; ++j) {
          if (_bs_closeEnough_8U_C3R(pFrame, pModel, step, matchTh, matchNum)) {
            *sm = bgColor;
            isUpdateModel = (_bs_getNextRandomNumber(p) % updateFactor);
            if (isUpdateModel) {
              _bs_updatePixelModel_8U_C3R(p, *pFrame, w, h, j,i);
            }
          } else {
            *sm = foreColor;
          }
          pModel += step;
          pFrame += 3;
          ++ sm;
        }
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
  free (p->randTable);
  free (p->bgModel);
  free (p->segMap);
  free(*t);
  *t = NULL;
}

/********geters && setter***********/

int bs_getSampleNum(const bst_t p) {
  return p->sampleNum;
}

int bs_getMatchNum(const bst_t p) {
  p->matchNumber;
}

int bs_getMatchThreshold(const bst_t p) {
  return p->matchThresh;
}

int bs_getUpdateFactor(const bst_t p) {
  return p->updateFactor;
}

int bs_getColorMode(const bst_t p) {
  return p->colorMode;
}

unsigned int bs_getRandTableLen(const bst_t p) {
  return p->randTableLen;
}

/* be careful with this */
unsigned char* bs_getModelPtr(const bst_t p) {
  return p->bgModel;
}

unsigned char* bs_getSegMapPtr(const bst_t p) {
  return p->segMap;
}

unsigned char bs_getBackgroundColor(const bst_t p) {
  return p->backgroundColor;
}

unsigned char bs_getForegroundColor(const bst_t p) {
  return p->foregroundColor;
}

int bs_getImageChannelNum(const bst_t p) {
  return p->imageChannelNum;
}

void bs_setForegroundColor(bst_t p, unsigned char c) {
  p->foregroundColor = c;
}

void bs_setBackgroundColor(bst_t p, unsigned char c) {
  p->backgroundColor = c;
}

void bs_setSampleNum(bst_t p, int n) {
  p->sampleNum = n;
}

void bs_setMatchNum(bst_t p, int n) {
  p->matchNum = n;
}

void bs_setMatchThreshold(bst_t p, int th) {
  p->matchThreshold = th;
}

void bs_setUpdateFactor(bst_t p, int f) {
  p->updateFactor = f;
}

void bs_setRandTableLen(bst_t p, int len) {
  p->randTableLen = len;
}

/* It's dangerous to provide those two APIs, but
 * the whole thing was created in C, you can't
 * avoid it, 'C trust programmers'.
 */
void bs_setModelPtr(bst_t p, unsigned char *newModel) {
  free (p->bgModel);
  p->bgModel = newModel;
}

void bs_setSegMapPtr(bst_t p, unsigned char *newSegMap) {
  free(p->segMap);
  p->segMap = newSegMap;
}

EXTERN_END
