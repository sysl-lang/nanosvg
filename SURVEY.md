# nanosvg.h: the survey

Every element of the public surface of `nanosvg.h` (upstream commit `239e102e`, 3278 lines of which
the declarations are lines 74-187; the rest is the body behind `NANOSVG_IMPLEMENTATION`), classified
as the organisation's survey asks:

1. **expressible**: scalars, pointers, out-params, a struct by value;
2. **shim-able**: no sysl spelling worth the risk (a union, a `char[N]` member, a `static`), a
   flattening to scalars is ordinary engineering;
3. **needs the language to move**: the bucket the survey exists to find.

**Nothing landed in bucket 3.** The binding wrote no workaround for a language gap.

## Functions (4)

| element | bucket | what the binding does |
|---|---|---|
| `nsvgParse(char *input, const char *units, float dpi)` | 2, shim | Takes a **mutable, NUL-terminated** buffer that nanosvg edits in place, and a C string for the unit. `sysl_nsvg_parse` takes the unit as a number and picks the literal in C, so no string crosses; the pleasant `parse` copies the sysl `string` into a `Buf[u8]`, pushes a `0`, and keeps it alive across the call. A sysl `string` is neither mutable nor terminated, and a test slices a longer string to prove the copy is. |
| `nsvgParseFromFile(const char *filename, ...)` | absent | Reads a file with `fopen`. Reading is `sysl.fs`'s job and the bytes it returns are what `parse_bytes` wants. It is compiled in (nanosvg has no switch for it) and never called. |
| `nsvgDelete(NSVGimage *)` | 1 | Bound as itself. It frees the whole tree, so ownership is one `Drop` on `Image`. |
| `nsvgDuplicatePath(NSVGpath *)` | absent | Returns a `malloc`ed path that is in no image, and upstream has no function to free one (`free(p->pts); free(p)` is by hand). Binding it would be binding a leak. `Path.points()` plus a sysl copy does the job. |

Ownership, the first of the four questions: the image is **one tree nanosvg allocates and
`nsvgDelete` frees**, so this is *not* the "C returns malloc'd storage the caller frees" shape that
needed qoi's arena: the C side supplies its own destructor. `Shape` and `Path` are interior pointers
and each holds the `&Image`, so the tree outlives every view of it.

## Enums (7)

| element | bucket | what the binding does |
|---|---|---|
| `NSVGpaintType` | 1 | `c const`; `Paint` = `None`, `Solid(Color)`, `LinearGradient`, `RadialGradient` (`UNDEF` is resolved to `NONE` by the time a parse returns). |
| `NSVGspreadType` | absent | Gradient only. |
| `NSVGlineJoin`, `NSVGlineCap` | 1 | `c const`; `LineJoin`, `LineCap`. |
| `NSVGfillRule` | 1 | `c const`; `FillRule`. |
| `NSVGflags` | 1 | `NSVG_FLAGS_VISIBLE` is tested in the shim; `Shape.visible()`. |
| `NSVGpaintOrder` | absent | Fill / markers / stroke order for rasterising; nothing a modeller extrudes. |

## Structs (6)

| element | bucket | what the binding does |
|---|---|---|
| `NSVGimage` (`width`, `height`, `shapes`) | 1 via accessors | `Image`. **`width` / `height` are in CSS pixels whatever unit was asked for**; the pleasant layer converts them (see README, *Units*). |
| `NSVGshape` | 2 | Every field read through an accessor, checked by the C compiler against the real layout: it holds an `NSVGpaint` with an **anonymous union**, three `char[64]` strings, `char` fields that are small integers, and arrays. Bound: `id`, fill, stroke, opacity, stroke width, join, cap, miter limit, dash array / count / offset, fill rule, visible flag, bounds, paths. Absent: `paintOrder`, `fillGradient` / `strokeGradient` names, `xform`. |
| `NSVGpath` | 1 via accessors | `Path`: `pts` (a `*f32` read as a `[]const f32` view of `2 * npts`), `npts`, `closed`, `bounds`. |
| `NSVGpaint` | 2 | A **tagged anonymous union** (`color` or `gradient*`). The shim returns the colour only when the type says colour. |
| `NSVGgradient`, `NSVGgradientStop` | absent | A gradient has a variable-length `stops[1]` tail (`malloc(sizeof + n-1 stops)`), a transform and a spread. A printed solid has no use for one; `Paint` reports that a gradient is there. Expressible if wanted (a flexible array member is a pointer plus a count). |

## Constants and macros

`NSVG_PI`, `NSVG_KAPPA90`, `NSVG_RGB(...)` and the rest are inside the `NANOSVG_IMPLEMENTATION` guard
and are not interface. `NSVG_RGB`'s layout (`r | g << 8 | b << 16`, alpha in the top byte) is
documented in the body and is unpacked by `unpack` in the pleasant layer, with a test.

## Compile-time options

None are consulted. `NANOSVG_ALL_COLOR_KEYWORDS` (line 1339) switches the table of colour names the
parser recognises from ten (`red green blue yellow cyan magenta black grey gray white`) to the full
CSS list; it changes **what the parser accepts and no declaration**, so this package leaves it
off. Hex and `rgb()` colours, which is what editors write, are unaffected; a document that says
`fill='orange'` gets nanosvg's fallback for a name it does not know (a test pins which).
`NANOSVG_CPLUSPLUS` concerns `extern "C"` only.

## `nanosvgrast.h` is deliberately absent

The rasteriser (`nsvgCreateRasterizer`, `nsvgRasterize`, `nsvgDeleteRasterizer`) is four functions
and *would* be cheap to bind: a handle with a `Drop` and a `u8` output buffer, all bucket 1. It is
left out because the use this package exists for is **outlines to extrude**, which the rasteriser
cannot help with: it fills pixels, never touches a bezier a caller can read back, and doing gradients
and strokes means a second vendored file (about 1,500 lines, its own allocator use, its own CVE
watch) for something `sh.sysl.stb` plus a drawing library already covers. It can be added later
without changing anything here.

## Four questions

- **Who owns the memory?** nanosvg, through `nsvgDelete`. No arena, no shim allocator.
- **Anything by value that is not a scalar?** No: every function takes and returns pointers or
  scalars.
- **Do compile-time options change the ABI?** No.
- **What capability?** `heap`. No file system, no clock, no network. Needs `-lm`.
