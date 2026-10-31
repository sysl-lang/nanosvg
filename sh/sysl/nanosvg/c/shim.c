/* nanosvg's body, and the small reads that keep its structs on the C side.
 *
 * `NANOSVG_IMPLEMENTATION` is the whole mechanism of a single-header library: the header is
 * declarations everywhere it is included and a body in exactly one place, and this file is that
 * place. Nothing about nanosvg is configured -- it has no compile-time options -- so there is no
 * `defines` block for this package.
 *
 * **Reading is by accessor rather than by laid-out sysl struct.** `NSVGshape` holds a
 * `NSVGpaint` whose union has to be read through its `type`, two `char[64]` gradient names, and
 * `char` members that are small integers; declaring that in sysl would be an agreement with the
 * header that nothing checks. An accessor is checked by the C compiler against the real struct.
 */

/* **Every block nanosvg allocates is counted**, so a test can say that dropping an image gave all of
 * them back. nanosvg calls `malloc`, `realloc` and `free` directly and has no allocator hook, so the
 * three are redefined for this translation unit only, after the system headers have declared them
 * and before nanosvg's body is read. The header itself stays byte for byte upstream's. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

static long long g_live_blocks = 0;

static void *counted_malloc(size_t n)
{
    void *p = malloc(n);
    if (p) __atomic_fetch_add(&g_live_blocks, 1, __ATOMIC_RELAXED);
    return p;
}

static void *counted_realloc(void *old, size_t n)
{
    void *p = realloc(old, n);
    if (p && !old) __atomic_fetch_add(&g_live_blocks, 1, __ATOMIC_RELAXED);
    return p;
}

static void counted_free(void *p)
{
    if (p) __atomic_fetch_sub(&g_live_blocks, 1, __ATOMIC_RELAXED);
    free(p);
}

#define malloc(n) counted_malloc(n)
#define realloc(p, n) counted_realloc(p, n)
#define free(p) counted_free(p)

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#include "shim.h"

#undef malloc
#undef realloc
#undef free

long long sysl_nsvg_live_blocks(void)
{
    return __atomic_load_n(&g_live_blocks, __ATOMIC_RELAXED);
}

NSVGimage *sysl_nsvg_parse(char *input, int units, float dpi)
{
    const char *name = "px";
    switch (units) {
    case SYSL_NSVG_UNITS_PT: name = "pt"; break;
    case SYSL_NSVG_UNITS_PC: name = "pc"; break;
    case SYSL_NSVG_UNITS_MM: name = "mm"; break;
    case SYSL_NSVG_UNITS_CM: name = "cm"; break;
    case SYSL_NSVG_UNITS_IN: name = "in"; break;
    default: break;
    }
    return nsvgParse(input, name, dpi);
}

float sysl_nsvg_image_width(const NSVGimage *image) { return image->width; }
float sysl_nsvg_image_height(const NSVGimage *image) { return image->height; }
NSVGshape *sysl_nsvg_image_shapes(const NSVGimage *image) { return image->shapes; }

NSVGshape *sysl_nsvg_shape_next(const NSVGshape *shape) { return shape->next; }
NSVGpath *sysl_nsvg_shape_paths(const NSVGshape *shape) { return shape->paths; }

const char *sysl_nsvg_shape_id(const NSVGshape *shape) { return shape->id; }

int sysl_nsvg_shape_fill_type(const NSVGshape *shape) { return shape->fill.type; }
unsigned int sysl_nsvg_shape_fill_color(const NSVGshape *shape)
{
    return shape->fill.type == NSVG_PAINT_COLOR ? shape->fill.color : 0;
}
int sysl_nsvg_shape_stroke_type(const NSVGshape *shape) { return shape->stroke.type; }
unsigned int sysl_nsvg_shape_stroke_color(const NSVGshape *shape)
{
    return shape->stroke.type == NSVG_PAINT_COLOR ? shape->stroke.color : 0;
}

float sysl_nsvg_shape_opacity(const NSVGshape *shape) { return shape->opacity; }
float sysl_nsvg_shape_stroke_width(const NSVGshape *shape) { return shape->strokeWidth; }
int sysl_nsvg_shape_line_join(const NSVGshape *shape) { return shape->strokeLineJoin; }
int sysl_nsvg_shape_line_cap(const NSVGshape *shape) { return shape->strokeLineCap; }
float sysl_nsvg_shape_miter_limit(const NSVGshape *shape) { return shape->miterLimit; }
int sysl_nsvg_shape_fill_rule(const NSVGshape *shape) { return shape->fillRule; }
int sysl_nsvg_shape_visible(const NSVGshape *shape)
{
    return (shape->flags & NSVG_FLAGS_VISIBLE) != 0;
}
int sysl_nsvg_shape_dash_count(const NSVGshape *shape) { return shape->strokeDashCount; }
float sysl_nsvg_shape_dash_offset(const NSVGshape *shape) { return shape->strokeDashOffset; }
float sysl_nsvg_shape_dash(const NSVGshape *shape, int i)
{
    return (i >= 0 && i < shape->strokeDashCount) ? shape->strokeDashArray[i] : 0.0f;
}

void sysl_nsvg_shape_bounds(const NSVGshape *shape, float *out)
{
    for (int i = 0; i < 4; i++) out[i] = shape->bounds[i];
}

NSVGpath *sysl_nsvg_path_next(const NSVGpath *path) { return path->next; }
const float *sysl_nsvg_path_pts(const NSVGpath *path) { return path->pts; }
int sysl_nsvg_path_npts(const NSVGpath *path) { return path->npts; }
int sysl_nsvg_path_closed(const NSVGpath *path) { return path->closed; }
void sysl_nsvg_path_bounds(const NSVGpath *path, float *out)
{
    for (int i = 0; i < 4; i++) out[i] = path->bounds[i];
}
