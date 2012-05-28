#include "bs2.h"
#include "mtwist.h" /* random number generator */
#include <stdlib.h>

#ifdef NDEBUG
#define bs_log(msg)
#else
#include <stdio.h>
#define bs_log(msg) fprintf(stderr, "%s", msg)
#endif 

#ifndef ABS
#define ABS(v) ((v) > 0 ? (v) : -(v))
#endif

EXTERN_BEGIN

struct _bs_t {
        int sampleNum;
        int matchThresh;
        int matchNum;
        int updateFactor;
        int imageWidth;
        int imageWidthStep;
        int imageHeight;
        int nchannels;
        uchar foregroundColor;
        uchar backgroundColor;

        unsigned int randTableLen;
        unsigned int *randTable;
        unsigned int randTableIdx;

        unsigned int bgModelWidthStep;
        uchar *bgModel;
        
        /* update method */
        void (* _update)(struct _bs_t *t, const uchar *data, uchar *segmap);
};


#define MT_RAND_MAX 0x7FFFFFFF /* 32 bit */

static int _mt_rand_in_range(unsigned int limit) {
        
        unsigned int divisor = MT_RAND_MAX / (limit + 1);
        unsigned int retval;
        do {
                retval = mt_lrand() / divisor; 
        } while( retval > limit );
        
        return retval;
}

static void _mt_gen_randTab(unsigned int arr[], unsigned int n, unsigned int limit) {
        /* generate random integer in [0, limit] */
        unsigned int i;
        if (limit < 1) return ;
        mt_goodseed();
        for (i = 0; i < n; ++i) {
                arr[i] = _mt_rand_in_range(limit);
        }
}

static int _bs_createRandTable(bs_t *t) {
        if (!t || !t->randTable) return -1;
        _mt_gen_randTab(t->randTable, t->randTableLen, 0xFFFF);
        t->randTableIdx = 0;
        return 0;
}

static unsigned int _bs_getNextRandomNumber(bs_t *t) {
        unsigned int v = t->randTable[ t->randTableIdx++ ];
        if ( t->randTableIdx == t->randTableLen )
                t->randTableIdx = 0;
        return v;
}

static void _bs_getRandomNeighbour_8U_C1R(bs_t *t, const uchar *data, int x, int y, uchar *v) {
        int nx = x, ny = y;
        int w = t->imageWidth, h = t->imageHeight;
        /*
          5 6 7
          4 i 0
          3 2 1
        */
#define _get_random_idx()                               \
        switch ( _bs_getNextRandomNumber(t) % 8 ) {     \
        case 0:                                         \
                nx ++;                                  \
                if (nx == w) nx --;                     \
                break;                                  \
        case 1:                                         \
                nx ++; ny ++;                           \
                if (nx == w) nx --;                     \
                if (ny == h) ny --;                     \
                break;                                  \
        case 2:                                         \
                ny ++;                                  \
                if (ny == h) ny --;                     \
                break;                                  \
        case 3:                                         \
                ny ++;                                  \
                if (nx) nx --;                          \
                if (ny == h ) ny --;                    \
                break;                                  \
        case 4:                                         \
                if (nx) nx --;                          \
                break;                                  \
        case 5:                                         \
                if (nx) nx --;                          \
                if (ny) ny --;                          \
                break;                                  \
        case 6:                                         \
                if (ny) ny--;                           \
                break;                                  \
        case 7:                                         \
                nx ++;                                  \
                if (nx == w) nx --;                     \
                if (ny) ny --;                          \
                break;                                  \
        }

        *v = data[nx + ny * t->imageWidthStep];
}

static void _bs_getRandomNeighbour_8U_C3R(bs_t *t, const uchar *data, int x, int y,
                                          uchar *v0, uchar *v1, uchar *v2) {
        int nx = x, ny = y;
        int w = t->imageWidth, h = t->imageHeight;
        int tmp;
        
        _get_random_idx();

        tmp = nx*3 + ny * t->imageWidthStep;
        *v0 = data[tmp + 0];
        *v1 = data[tmp + 1];
        *v2 = data[tmp + 2];
}

static void _bs_updatePixelModel_8U_C1R(bs_t *t, const uchar *pval, int x, int y) {
        int nx = x, ny = y, samplenum = t->sampleNum;
        int w = t->imageWidth, h = t->imageHeight;
        unsigned int memberToReplace = _bs_getNextRandomNumber(t) % samplenum;
        unsigned int bws = t->bgModelWidthStep;
        
        t->bgModel[ nx * samplenum + ny * bws + memberToReplace ] = *pval;

        _get_random_idx();

        t->bgModel[ nx * samplenum + ny * bws + memberToReplace ] = *pval;
}

static void _bs_updatePixelModel_8U_C3R(bs_t *t, const uchar *pval, int x, int y) {
        int nx = x, ny = y, samplenum = t->sampleNum;
        int w = t->imageWidth, h = t->imageHeight;
        unsigned int memberToReplace = _bs_getNextRandomNumber(t) % samplenum;
        unsigned int bws = t->bgModelWidthStep;
        unsigned int idx = nx*samplenum*3 + ny * bws + memberToReplace * 3;

        t->bgModel[ idx + 0 ] = pval[0];
        t->bgModel[ idx + 1 ] = pval[1];
        t->bgModel[ idx + 2 ] = pval[2];

        _get_random_idx();

        idx = nx*samplenum*3 + ny * bws + memberToReplace * 3;
        t->bgModel[ idx + 0 ] = pval[0];
        t->bgModel[ idx + 1 ] = pval[1];
        t->bgModel[ idx + 2 ] = pval[2];
}

#undef _get_random_idx

static int _bs_closeEnough_8U_C1R(const bs_t *t, const uchar *data, const uchar *pmodel) {
        int k,delta,count = 0;
        int samplenum = t->sampleNum;
        for (k = 0; k < samplenum; ++k) {
                delta = ABS(data[0] - pmodel[k]);
                if (delta < t->matchThresh)
                        if (++count >= t->matchNum)
                                return 1;
        }
        return 0;
}

static int _bs_closeEnough_8U_C3R(const bs_t *t, const uchar *data, const uchar *pmodel) {
        int k, n, th, delta, count = 0;
        uchar v0, v1, v2;
        
        n = t->sampleNum * 3;
        th = t->matchThresh * 3;
        
        v0 = data[0];
        v1 = data[1];
        v2 = data[2];
        
        for (k = 0; k < n; k += 3) {
                delta  = ABS(v0 - pmodel[k + 0]);
                delta += ABS(v1 - pmodel[k + 1]);
                delta += ABS(v2 - pmodel[k + 2]);
                if (delta < th)
                        if (++count >= t->matchNum)
                                return 1;
        }
        return 0;
}

/* @NOTE the width of segmap should == the image width 4-aligned */
static void _bs_updateModel_8U_C1R(bs_t *t, const uchar *data, uchar *segmap) {
        int x,y,w,ws,h, bws, factor, sws,samplenum;
        uchar fcolor = t->foregroundColor, bcolor = t->backgroundColor;
        uchar *pseg = segmap;
        uchar *pmodel = t->bgModel;
        const uchar *pdata = data;
        ws = t->imageWidthStep;
        bws = t->bgModelWidthStep;
        w = t->imageWidth;
        h = t->imageHeight;
        sws = (w + 3) & (~3);
        factor = t->updateFactor;
        samplenum = t->sampleNum;
        
        for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                        pseg[x + y*sws] = fcolor;
                        if ( _bs_closeEnough_8U_C1R(t, &pdata[x + y*ws], &pmodel[x*samplenum + y*bws]) ) {
                                pseg[x + y*sws] = bcolor;
                                if ( (_bs_getNextRandomNumber(t) % factor) == (factor - 1)) {
                                        _bs_updatePixelModel_8U_C1R(t, &pdata[x + y*ws], x, y);
                                }
                        }
                }
        }
}

static void _bs_updateModel_8U_C3R(bs_t *t, const uchar *data, uchar *segmap) {
        int x,y,w,ws,h, bws, factor,sws;
        
        uchar fcolor = t->foregroundColor, bcolor = t->backgroundColor;
        uchar *pseg = segmap;
        uchar *pmodel = t->bgModel;
        const uchar *pdata = data;

        bws = t->bgModelWidthStep;
        w = t->imageWidth;
        h = t->imageHeight;
        ws = t->imageWidthStep;
        sws = (w + 3) & (~3);
        factor = t->updateFactor;
        
        for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                        pseg[x + y*sws] = fcolor;
                        if ( _bs_closeEnough_8U_C3R(t, &pdata[x*3 + y*ws], &pmodel[x*t->sampleNum*3 + y*bws]) ) {
                                pseg[x + y*sws] = bcolor;
                                if ( (_bs_getNextRandomNumber(t) % factor) == (factor - 1)) {
                                        _bs_updatePixelModel_8U_C3R(t, &pdata[x*3 + y*ws], x, y);
                                }
                        }
                }
        }
}


static void _bs_initModel_8U_C1R(bs_t *t, const uchar *data) {
        int w, ws, h, bws, samplenum, x, y, k;
        uchar *pmodel = t->bgModel;
        uchar v = 0;

        w = t->imageWidth;
        ws = t->imageWidthStep;
        h = t->imageHeight;
        bws = t->bgModelWidthStep;
        samplenum = t->sampleNum;
        
        for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                        pmodel[y*bws + x*samplenum + 0] = data[y*ws + x];
                        for (k = 1; k < samplenum; ++k) {
                                _bs_getRandomNeighbour_8U_C1R(t, data, x, y, &v);
                                pmodel[y*bws + x*samplenum + k] = v;
                        }
                }
        }
}

static void _bs_initModel_8U_C3R(bs_t *t, const uchar *data) {
        int w, ws, h, bws, samplenum, x, y, k;
        uchar *pmodel = t->bgModel;
        uchar v0 = 0, v1 = 0, v2 = 0;

        w = t->imageWidth;
        ws = t->imageWidthStep;
        h = t->imageHeight;
        bws = t->bgModelWidthStep;
        samplenum = t->sampleNum;
        
        for (y = 0; y < h; ++y) {
                for (x = 0; x < w; ++x) {
                        pmodel[y*bws + x*samplenum*3 + 0] = data[y*ws + x*3 + 0];
                        pmodel[y*bws + x*samplenum*3 + 1] = data[y*ws + x*3 + 1];
                        pmodel[y*bws + x*samplenum*3 + 2] = data[y*ws + x*3 + 2];
                        for (k = 3; k < samplenum*3; k+=3) {
                                _bs_getRandomNeighbour_8U_C3R(t, data, x, y, &v0, &v1, &v2);
                                pmodel[y*bws + x*samplenum*3 + k + 0] = v0;
                                pmodel[y*bws + x*samplenum*3 + k + 1] = v1;
                                pmodel[y*bws + x*samplenum*3 + k + 2] = v2;
                        }
                }
        }
}


bs_t* bs_create(void) {
        bs_t *t = (bs_t *)malloc(sizeof(bs_t));

        if (!t) {
                bs_log("No sufficient memory!\n");
                return NULL;
        }
        /* default parameters */
        t->sampleNum = 20;
        t->matchThresh = 20;
        t->matchNum = 2;
        t->updateFactor = 16;
        t->foregroundColor = 0xFF;
        t->backgroundColor = 0x00;
        t->imageWidth = 0;
        t->imageWidthStep = 0;
        t->imageHeight = 0;
        t->nchannels = 0;
        t->randTableLen = 0xFFFF;
        t->randTable = NULL;
        t->randTableIdx = 0;
        
        t->bgModelWidthStep = 0;
        t->bgModel = NULL;

        return t;
}

int bs_init(bs_t *t, const uchar *data, int w, int ws, int h, int nchannels) {
        
        if (!t || !data || (w < 1) || (h < 1)) {
                bs_log("Invalid parameter(s)!\n");
                return -1;
        }

        if (nchannels != 1 && nchannels != 3) {
                bs_log("Only support 1 or 3 channels image!\n");
                return -1;
        }
        
        t->imageWidth = w;
        t->imageWidthStep = ws;
        t->imageHeight = h;
        t->nchannels = nchannels;

        free( t->randTable );
        t->randTable = (unsigned int*)malloc(sizeof(unsigned int) * t->randTableLen);
        _bs_createRandTable(t);

        free( t->bgModel );
        /* assume 4 bytes as the default align size */
        t->bgModelWidthStep = (t->sampleNum * nchannels * w + 3) & (~3);
        t->bgModel = (uchar *)calloc(t->bgModelWidthStep * h, sizeof(uchar));
        if (!t->bgModel) {
                bs_log("No sufficient memory!\n");
                return -1;
        }

        switch( nchannels ) {
        case 1:
                _bs_initModel_8U_C1R(t, data);
                t->_update = &_bs_updateModel_8U_C1R;
                break;
        case 3:
                _bs_initModel_8U_C3R(t, data);
                t->_update = &_bs_updateModel_8U_C3R;
                break;
        }
       
        return 0;
}

uchar* bs_create_segmap(int w, int h) {
        w = (w + 3) & (~3);
        return  (uchar *)malloc(sizeof(uchar) * w * h);
}

int bs_update(bs_t *t, const uchar *data, uchar *segmap) {
        if (!t || !t->bgModel) {
                bs_log("Invalid parameters!\n");
                return -1;
        }
        
        t->_update(t, data, segmap);
        return 0;
}

void bs_destroy(bs_t **t) {
        if (!t || !(*t)) return ;
        free( (*t)->randTable );
        free( (*t)->bgModel );
        free( *t );
        *t = NULL;
}

EXTERN_END
