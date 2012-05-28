/**
 * refactored version of bs.
 *
 * @author blackball (bugway@gmail.com)
 */

#ifndef BS2_H
#define BS2_H

#ifdef __cplusplus
#define EXTERN_BEGIN extern "C" {
#define EXTERN_END   }
#else
#define EXTERN_BEGIN
#define EXTERN_END
#endif

EXTERN_BEGIN
typedef unsigned char uchar;
typedef struct _bs_t bs_t;

bs_t*  bs_create(void);
int    bs_init(bs_t *t, const uchar *data, int w, int ws, int h, int nchannels);
uchar* bs_create_segmap(int w, int h); /* create a proper aligned segmap */
int    bs_update(bs_t *t, const uchar *data, uchar *segmap);
void   bs_destroy(bs_t **t);

EXTERN_END

#endif
