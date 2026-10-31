/* What `shim.c` answers, said once so the sysl side and the C side read the same list.
 *
 * Every function here exists because the thing it reads has no sysl spelling worth the risk: a
 * struct with an anonymous union inside it (`NSVGpaint`), a `char[64]` member, a `char` that is
 * really a small integer, and a `const char *` unit name that sysl would have to keep alive and
 * terminate. Reading them in C costs one call each and cannot disagree with `nanosvg.h`'s layout.
 */

#ifndef SYSL_NANOSVG_SHIM_H
#define SYSL_NANOSVG_SHIM_H

#include "nanosvg.h"

/* Which unit the parsed coordinates come out in. The numbers are the shim's own and the sysl side
 * asks for them by name. */
#define SYSL_NSVG_UNITS_PX 0
#define SYSL_NSVG_UNITS_PT 1
#define SYSL_NSVG_UNITS_PC 2
#define SYSL_NSVG_UNITS_MM 3
#define SYSL_NSVG_UNITS_CM 4
#define SYSL_NSVG_UNITS_IN 5

/* `nsvgParse`, with the unit chosen by number so no C string crosses the boundary. `input` must be
 * NUL-terminated and is edited in place. Answers null when nanosvg could not allocate. */
NSVGimage *sysl_nsvg_parse(char *input, int units, float dpi);

/* How many blocks nanosvg has allocated and not yet freed, across every image. */
long long sysl_nsvg_live_blocks(void);

float sysl_nsvg_image_width(const NSVGimage *image);
float sysl_nsvg_image_height(const NSVGimage *image);
NSVGshape *sysl_nsvg_image_shapes(const NSVGimage *image);

NSVGshape *sysl_nsvg_shape_next(const NSVGshape *shape);
NSVGpath *sysl_nsvg_shape_paths(const NSVGshape *shape);

/* The shape's id, a NUL-terminated string of at most 63 bytes inside the shape. */
const char *sysl_nsvg_shape_id(const NSVGshape *shape);

int sysl_nsvg_shape_fill_type(const NSVGshape *shape);
unsigned int sysl_nsvg_shape_fill_color(const NSVGshape *shape);
int sysl_nsvg_shape_stroke_type(const NSVGshape *shape);
unsigned int sysl_nsvg_shape_stroke_color(const NSVGshape *shape);

float sysl_nsvg_shape_opacity(const NSVGshape *shape);
float sysl_nsvg_shape_stroke_width(const NSVGshape *shape);
int sysl_nsvg_shape_line_join(const NSVGshape *shape);
int sysl_nsvg_shape_line_cap(const NSVGshape *shape);
float sysl_nsvg_shape_miter_limit(const NSVGshape *shape);
int sysl_nsvg_shape_fill_rule(const NSVGshape *shape);
int sysl_nsvg_shape_visible(const NSVGshape *shape);
int sysl_nsvg_shape_dash_count(const NSVGshape *shape);
float sysl_nsvg_shape_dash_offset(const NSVGshape *shape);
float sysl_nsvg_shape_dash(const NSVGshape *shape, int i);

/* The shape's tight bounding box, written as minx, miny, maxx, maxy. */
void sysl_nsvg_shape_bounds(const NSVGshape *shape, float *out);

NSVGpath *sysl_nsvg_path_next(const NSVGpath *path);
const float *sysl_nsvg_path_pts(const NSVGpath *path);
int sysl_nsvg_path_npts(const NSVGpath *path);
int sysl_nsvg_path_closed(const NSVGpath *path);
void sysl_nsvg_path_bounds(const NSVGpath *path, float *out);

#endif
