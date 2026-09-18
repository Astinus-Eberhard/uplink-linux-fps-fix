#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Minimal GL types/constants — no dependency on libgl-dev headers. */
typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef int GLint;
typedef unsigned int GLuint;
typedef int GLsizei;
typedef float GLfloat;
typedef double GLdouble;
typedef unsigned char GLubyte;
typedef void GLvoid;

#define GL_TEXTURE_2D                     0x0DE1
#define GL_ALPHA                          0x1906
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_BLEND                          0x0BE2
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303
#define GL_TEXTURE_ENV                    0x2300
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_MODULATE                       0x2100
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_NEAREST                        0x2600
#define GL_UNPACK_ALIGNMENT               0x0CF5
#define GL_CURRENT_RASTER_POSITION        0x0B07
#define GL_CURRENT_RASTER_POSITION_VALID  0x0B08
#define GL_CURRENT_COLOR                  0x0B00
#define GL_QUADS                          0x0007
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803
#define GL_CLAMP                          0x2900
#define GL_PROJECTION                     0x1701
#define GL_MODELVIEW                      0x1700
#define GL_VIEWPORT                       0x0BA2
#define GL_MATRIX_MODE                    0x0BA0
#define GL_ALL_ATTRIB_BITS                0xFFFFFFFFu

typedef void (*glBitmap_t)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte*);
typedef void (*glMatrixMode_t)(GLenum);
typedef void (*glPushMatrix_t)(void);
typedef void (*glPopMatrix_t)(void);
typedef void (*glLoadIdentity_t)(void);
typedef void (*glOrtho_t)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
typedef void (*glPushAttrib_t)(unsigned int);
typedef void (*glPopAttrib_t)(void);
typedef void (*glGetFloatv_t)(GLenum, GLfloat*);
typedef void (*glGetBooleanv_t)(GLenum, GLboolean*);
typedef void (*glGenTextures_t)(GLsizei, GLuint*);
typedef void (*glBindTexture_t)(GLenum, GLuint);
typedef void (*glTexImage2D_t)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid*);
typedef void (*glTexParameteri_t)(GLenum, GLenum, GLint);
typedef void (*glTexEnvf_t)(GLenum, GLenum, GLfloat);
typedef void (*glEnable_t)(GLenum);
typedef void (*glDisable_t)(GLenum);
typedef void (*glBlendFunc_t)(GLenum, GLenum);
typedef void (*glBegin_t)(GLenum);
typedef void (*glEnd_t)(void);
typedef void (*glVertex2f_t)(GLfloat, GLfloat);
typedef void (*glTexCoord2f_t)(GLfloat, GLfloat);
typedef void (*glColor4fv_t)(const GLfloat*);
typedef void (*glPixelStorei_t)(GLenum, GLint);
typedef void (*glGetIntegerv_t)(GLenum, GLint*);

#define GL_UNPACK_LSB_FIRST               0x0CF1
#define GL_UNPACK_ROW_LENGTH              0x0CF2
#define GL_UNPACK_SKIP_ROWS               0x0CF3
#define GL_UNPACK_SKIP_PIXELS             0x0CF4

static glBitmap_t real_glBitmap;
static glGetFloatv_t real_glGetFloatv;
static glGetBooleanv_t real_glGetBooleanv;
static glGenTextures_t real_glGenTextures;
static glBindTexture_t real_glBindTexture;
static glTexImage2D_t real_glTexImage2D;
static glTexParameteri_t real_glTexParameteri;
static glTexEnvf_t real_glTexEnvf;
static glEnable_t real_glEnable;
static glDisable_t real_glDisable;
static glBlendFunc_t real_glBlendFunc;
static glBegin_t real_glBegin;
static glEnd_t real_glEnd;
static glVertex2f_t real_glVertex2f;
static glTexCoord2f_t real_glTexCoord2f;
static glColor4fv_t real_glColor4fv;
static glPixelStorei_t real_glPixelStorei;
static glGetIntegerv_t real_glGetIntegerv;
static glMatrixMode_t real_glMatrixMode;
static glPushMatrix_t real_glPushMatrix;
static glPopMatrix_t real_glPopMatrix;
static glLoadIdentity_t real_glLoadIdentity;
static glOrtho_t real_glOrtho;
static glPushAttrib_t real_glPushAttrib;
static glPopAttrib_t real_glPopAttrib;

static int inited = 0;

typedef struct CacheEntry {
    uint64_t hash;
    GLsizei width, height;
    GLfloat xorig, yorig;
    GLuint texId;
    struct CacheEntry *next;
} CacheEntry;

#define HASH_BUCKETS 1024
static CacheEntry *buckets[HASH_BUCKETS];
static long cache_hits = 0, cache_misses = 0;

static uint64_t fnv1a(const void *data, size_t len) {
    const unsigned char *p = (const unsigned char*)data;
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

static void init_syms(void) {
    if (inited) return;
    real_glBitmap = (glBitmap_t)dlsym(RTLD_NEXT, "glBitmap");
    real_glGetFloatv = (glGetFloatv_t)dlsym(RTLD_NEXT, "glGetFloatv");
    real_glGetBooleanv = (glGetBooleanv_t)dlsym(RTLD_NEXT, "glGetBooleanv");
    real_glGenTextures = (glGenTextures_t)dlsym(RTLD_NEXT, "glGenTextures");
    real_glBindTexture = (glBindTexture_t)dlsym(RTLD_NEXT, "glBindTexture");
    real_glTexImage2D = (glTexImage2D_t)dlsym(RTLD_NEXT, "glTexImage2D");
    real_glTexParameteri = (glTexParameteri_t)dlsym(RTLD_NEXT, "glTexParameteri");
    real_glTexEnvf = (glTexEnvf_t)dlsym(RTLD_NEXT, "glTexEnvf");
    real_glEnable = (glEnable_t)dlsym(RTLD_NEXT, "glEnable");
    real_glDisable = (glDisable_t)dlsym(RTLD_NEXT, "glDisable");
    real_glBlendFunc = (glBlendFunc_t)dlsym(RTLD_NEXT, "glBlendFunc");
    real_glBegin = (glBegin_t)dlsym(RTLD_NEXT, "glBegin");
    real_glEnd = (glEnd_t)dlsym(RTLD_NEXT, "glEnd");
    real_glVertex2f = (glVertex2f_t)dlsym(RTLD_NEXT, "glVertex2f");
    real_glTexCoord2f = (glTexCoord2f_t)dlsym(RTLD_NEXT, "glTexCoord2f");
    real_glColor4fv = (glColor4fv_t)dlsym(RTLD_NEXT, "glColor4fv");
    real_glPixelStorei = (glPixelStorei_t)dlsym(RTLD_NEXT, "glPixelStorei");
    real_glGetIntegerv = (glGetIntegerv_t)dlsym(RTLD_NEXT, "glGetIntegerv");
    real_glMatrixMode = (glMatrixMode_t)dlsym(RTLD_NEXT, "glMatrixMode");
    real_glPushMatrix = (glPushMatrix_t)dlsym(RTLD_NEXT, "glPushMatrix");
    real_glPopMatrix = (glPopMatrix_t)dlsym(RTLD_NEXT, "glPopMatrix");
    real_glLoadIdentity = (glLoadIdentity_t)dlsym(RTLD_NEXT, "glLoadIdentity");
    real_glOrtho = (glOrtho_t)dlsym(RTLD_NEXT, "glOrtho");
    real_glPushAttrib = (glPushAttrib_t)dlsym(RTLD_NEXT, "glPushAttrib");
    real_glPopAttrib = (glPopAttrib_t)dlsym(RTLD_NEXT, "glPopAttrib");
    inited = 1;
    fprintf(stderr, "[bitmapcache] glBitmap intercepted, caching glyphs as textures\n");
}

/* Converts a 1bpp bitmap (per current GL_UNPACK_* state, rows bottom-to-top per the
   OpenGL convention) into 8bpp alpha. Row 0 in the source data is the BOTTOM of the
   image -> we write it into the top of our output buffer so the resulting texture
   (TexCoord (0,0) = bottom) keeps the same orientation. */
static int compute_row_bytes(GLsizei width, GLint alignment) {
    int bits = width;
    int bytes = (bits + 7) / 8;
    if (alignment < 1) alignment = 1;
    return ((bytes + alignment - 1) / alignment) * alignment;
}

/* Per the GL spec, when GL_UNPACK_ROW_LENGTH != 0 it (not the width passed to glBitmap)
   determines the row stride in the source buffer. Missing this was a real bug: the
   game's small button font is packed with ROW_LENGTH > width (a fixed-size cell per
   glyph in a bitmap sheet), so computing stride from width alone read every other
   "row" as the padding byte of the real, wider stride — producing a ghost pattern of
   alternating blank rows in every decoded glyph. */
static unsigned char *convert_bitmap(const GLubyte *bitmap, GLsizei width, GLsizei height,
                                      GLint alignment, GLboolean lsb_first,
                                      GLint row_length, GLint skip_rows, GLint skip_pixels) {
    if (!bitmap || width <= 0 || height <= 0) return NULL;
    GLsizei stride_width = (row_length > 0) ? row_length : width;
    int row_bytes = compute_row_bytes(stride_width, alignment);
    unsigned char *out = malloc((size_t)width * height);
    if (!out) return NULL;
    for (int y = 0; y < height; y++) {
        const unsigned char *row = bitmap + (size_t)(y + skip_rows) * row_bytes;
        for (int x = 0; x < width; x++) {
            int bitidx = x + skip_pixels;
            int byte = row[bitidx / 8];
            int bitpos = bitidx % 8;
            int bit = lsb_first ? bitpos : (7 - bitpos);
            out[(size_t)y * width + x] = ((byte >> bit) & 1) ? 255 : 0;
        }
    }
    return out;
}

static CacheEntry *find_or_create(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
                                   const GLubyte *bitmap) {
    if (width <= 0 || height <= 0 || !bitmap) return NULL;

    GLint alignment = 4;
    GLboolean lsb_first = 0;
    GLint row_length = 0, skip_rows = 0, skip_pixels = 0;
    real_glGetIntegerv(GL_UNPACK_ALIGNMENT, &alignment);
    real_glGetBooleanv(GL_UNPACK_LSB_FIRST, &lsb_first);
    real_glGetIntegerv(GL_UNPACK_ROW_LENGTH, &row_length);
    real_glGetIntegerv(GL_UNPACK_SKIP_ROWS, &skip_rows);
    real_glGetIntegerv(GL_UNPACK_SKIP_PIXELS, &skip_pixels);

    GLsizei stride_width = (row_length > 0) ? row_length : width;
    int row_bytes = compute_row_bytes(stride_width, alignment);
    size_t bmp_len = (size_t)row_bytes * (height + skip_rows);
    uint64_t h = fnv1a(bitmap, bmp_len);
    h ^= ((uint64_t)(unsigned)width << 32) ^ (unsigned)height;
    h ^= ((uint64_t)(unsigned)alignment << 48) ^ ((uint64_t)(unsigned)lsb_first << 56);
    h ^= ((uint64_t)(unsigned)row_length << 24) ^ ((uint64_t)(unsigned)skip_pixels << 16) ^ (unsigned)skip_rows;
    unsigned bucket = (unsigned)(h % HASH_BUCKETS);

    for (CacheEntry *e = buckets[bucket]; e; e = e->next) {
        if (e->hash == h && e->width == width && e->height == height) {
            cache_hits++;
            return e;
        }
    }

    cache_misses++;
    unsigned char *alpha = convert_bitmap(bitmap, width, height, alignment, lsb_first,
                                           row_length, skip_rows, skip_pixels);
    if (!alpha) return NULL;

    GLuint tex = 0;
    real_glGenTextures(1, &tex);
    real_glBindTexture(GL_TEXTURE_2D, tex);
    real_glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* 'alpha' is OUR tightly-packed buffer (width bytes/row, no padding) — we must
       explicitly zero ROW_LENGTH/SKIP_* left over from the game's state (e.g. 16 for
       the small font), otherwise the exact same bug as decoding hits again on upload:
       glTexImage2D would read our buffer with the wrong stride. */
    real_glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    real_glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
    real_glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    real_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    real_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    real_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    real_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    real_glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, alpha);
    free(alpha);
    /* Restore the game's original unpack state — don't assume it resets this before
       EVERY glBitmap call. */
    real_glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
    real_glPixelStorei(GL_UNPACK_ROW_LENGTH, row_length);
    real_glPixelStorei(GL_UNPACK_SKIP_ROWS, skip_rows);
    real_glPixelStorei(GL_UNPACK_SKIP_PIXELS, skip_pixels);

    CacheEntry *e = malloc(sizeof(CacheEntry));
    e->hash = h;
    e->width = width;
    e->height = height;
    e->xorig = xorig;
    e->yorig = yorig;
    e->texId = tex;
    e->next = buckets[bucket];
    buckets[bucket] = e;
    return e;
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig,
              GLfloat xmove, GLfloat ymove, const GLubyte *bitmap) {
    init_syms();

    GLboolean valid = 1;
    real_glGetBooleanv(GL_CURRENT_RASTER_POSITION_VALID, &valid);

    GLfloat rasterPos[4] = {0, 0, 0, 1};
    if (valid) real_glGetFloatv(GL_CURRENT_RASTER_POSITION, rasterPos);

    GLfloat color[4] = {1, 1, 1, 1};
    if (valid && bitmap && width > 0 && height > 0) real_glGetFloatv(GL_CURRENT_COLOR, color);

    CacheEntry *e = (valid && bitmap) ? find_or_create(width, height, xorig, yorig, bitmap) : NULL;

    if (e) {
        /* rasterPos is already in WINDOW SPACE (the output of the full pipeline at the
           moment glRasterPos was called). Drawing it via glVertex2f sends it through the
           game's CURRENT modelview/projection matrices a second time -> the same point
           lands somewhere else. This was a real bug (three earlier fix attempts never
           touched it). Fix: while drawing the quad, swap in an identity modelview +
           a pixel-exact ortho matching the current viewport, so window-space coordinates
           land exactly where they should, then restore the game's original matrices
           unchanged. */
        GLint vp[4] = {0, 0, 0, 0};
        real_glGetIntegerv(GL_VIEWPORT, vp);
        GLint prevMatrixMode = GL_MODELVIEW;
        real_glGetIntegerv(GL_MATRIX_MODE, &prevMatrixMode);

        GLfloat x0 = (GLfloat)(long)(rasterPos[0] - xorig + (rasterPos[0] >= 0 ? 0.5f : -0.5f));
        GLfloat y0 = (GLfloat)(long)(rasterPos[1] - yorig + (rasterPos[1] >= 0 ? 0.5f : -0.5f));
        GLfloat x1 = x0 + width;
        GLfloat y1 = y0 + height;

        real_glPushAttrib(GL_ALL_ATTRIB_BITS);

        real_glMatrixMode(GL_PROJECTION);
        real_glPushMatrix();
        real_glLoadIdentity();
        real_glOrtho(vp[0], vp[0] + vp[2], vp[1], vp[1] + vp[3], -1.0, 1.0);
        real_glMatrixMode(GL_MODELVIEW);
        real_glPushMatrix();
        real_glLoadIdentity();

        real_glEnable(GL_TEXTURE_2D);
        real_glEnable(GL_BLEND);
        real_glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        real_glBindTexture(GL_TEXTURE_2D, e->texId);
        real_glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        real_glColor4fv(color);

        real_glBegin(GL_QUADS);
        real_glTexCoord2f(0, 0); real_glVertex2f(x0, y0);
        real_glTexCoord2f(1, 0); real_glVertex2f(x1, y0);
        real_glTexCoord2f(1, 1); real_glVertex2f(x1, y1);
        real_glTexCoord2f(0, 1); real_glVertex2f(x0, y1);
        real_glEnd();

        real_glMatrixMode(GL_PROJECTION);
        real_glPopMatrix();
        real_glMatrixMode(GL_MODELVIEW);
        real_glPopMatrix();
        real_glMatrixMode((GLenum)prevMatrixMode);

        real_glPopAttrib();

        /* Zero-size bitmap = zero rasterization cost, only advances the raster position
           per the GL spec. */
        real_glBitmap(0, 0, xorig, yorig, xmove, ymove, NULL);
    } else {
        /* Fallback: no cache entry (e.g. invalid raster pos) - original behavior. */
        real_glBitmap(width, height, xorig, yorig, xmove, ymove, bitmap);
    }
}
