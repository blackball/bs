/**
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
typedef struct _bs_t* bs_t;

/* create bs object using default parameters,
 * you could using the setter to change them.
 */
int bs_create(bs_t *t);
/* initialize background models */
int bs_initModel(bs_t *t, unsigned char *imagedata,
                 int w, int h, int nChannels);
/* perform background sbstraction and update model */
int bs_update(bs_t *t, unsigned char *segmap);
/* destroy */
int bs_destroy(bs_t *t);

int bs_getSampleNum(const bst_t p);
int bs_getMatchNum(const bst_t p);
int bs_getMatchThreshold(const bst_t p);
int bs_getUpdateFactor(const bst_t p);
int bs_getColorMode(const bst_t p);
unsigned int bs_getRandTableLen(const bst_t p);
unsigned char* bs_getModelPtr(const bst_t p);
unsigned char* bs_getSegMapPtr(const bst_t p);
unsigned char bs_getBackgroundColor(const bst_t p);
unsigned char bs_getForegroundColor(const bst_t p);
int bs_getImageChannelNum(const bst_t p);

void bs_setForegroundColor(bst_t p, unsigned char c);
void bs_setBackgroundColor(bst_t p, unsigned char c);
void bs_setSampleNum(bst_t p, int n);
void bs_setMatchNum(bst_t p, int n);
void bs_setMatchThreshold(bst_t p, int th);
void bs_setUpdateFactor(bst_t p, int f);
void bs_setRandTableLen(bst_t p, unsigned int len);

/* It's dangerous to provide those two APIs, but
 * the whole thing was created in C, you can't
 * avoid it, 'C trust programmers'.
 */
void bs_setModelPtr(bst_t p, unsigned char *newModel);
void bs_setSegMapPtr(bst_t p, unsigned char *newSegMap);


EXTERN_END

#endif
