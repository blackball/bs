/**
 * A more flexible C implementation of 'bgooth.h'
 *
 * @blackball @date 3/13/2012
 */

#ifndef C_BS_H
#define C_BS_H

#ifdef __cplusplus
#define EXTERN_BEGIN extern "C" {
#define EXTERN_END   }
#else
#define EXTERN_BEGIN
#define EXTERN_END
#endif

EXTERN_BEGIN

/*****another APIs pack *****/
typedef struct _bs_t *bs_t;
typedef unsigned char uchar;

int bs_create(bs_t *t);
int bs_initModel(bs_t *t, uchar *imagedata, int w, int ws, int h, int nChannels);
int bs_update(const bs_t *t, uchar *frameData, uchar *segmap);
int bs_destroy(bs_t *t);

/* Customization API */
int bs_getSampleNum(const bs_t p);
int bs_getMatchNum(const bs_t p);
int bs_getMatchThreshold(const bs_t p);
int bs_getUpdateFactor(const bs_t p);
int bs_getColorMode(const bs_t p);
int bs_getImageChannelNum(const bs_t p);
uchar* bs_getModelPtr(const bs_t p);
uchar* bs_getSegMapPtr(const bs_t p);
uchar bs_getBackgroundColor(const bs_t p);
uchar bs_getForegroundColor(const bs_t p);
unsigned int bs_getRandTableLen(const bs_t p);

void bs_setForegroundColor(const bs_t p, uchar c);
void bs_setBackgroundColor(const bs_t p, uchar c);
void bs_setSampleNum(const bs_t p, int n);
void bs_setMatchNum(const bs_t p, int n);
void bs_setMatchThreshold(const bs_t p, int th);
void bs_setUpdateFactor(const bs_t p, int f);
void bs_setRandTableLen(const bs_t p, unsigned int len);

/* It's dangerous to provide those two APIs, but
 * the whole thing was created in C, you can't
 * avoid it, 'C trust programmers'.
 */
void bs_setModelPtr(const bs_t p, unsigned char *newModel);
void bs_setSegMapPtr(const bs_t p, unsigned char *newSegMap);

EXTERN_END

#endif
