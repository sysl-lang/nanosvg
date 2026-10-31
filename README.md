# nanosvg

SVG outlines as cubic beziers for [sysl](https://github.com/sysl-lang/sysl), bound to
[nanosvg](https://github.com/memononen/nanosvg), with a flattener that turns them into polylines
within a tolerance you choose, in the units you choose.

*An independent sysl package for nanosvg. It is not affiliated with or endorsed by nanosvg's authors or trademark holders.*

nanosvg parses an SVG into shapes made of cubic bezier paths, with every primitive, arc and quadratic
converted, the `transform` and `viewBox` applied, and the result scaled into millimetres, inches or
pixels. That is what a modeller wants from an outline it is going to extrude, and what this package
adds is the last step: `flatten`, which turns a shape's curves into closed polygons accurate to a
tolerance, no further dependency needed.

```sysl
import sh.sysl.nanosvg.{parse, Units}

outline_points(text: string) -> Result[usize, Error]
    val img = parse(text, Units.Mm)?              // an image, in millimetres
    var total = 0
    for shape in img.shapes()
        if shape.visible()
            for outline in shape.flatten(0.01)?    // every vertex within 0.01 mm of the curve
                total += outline.points.len()      // also outline.closed, area(), perimeter()
    Ok(total)
```

(`tests.sysl` compiles and runs exactly this function.)

## Using it

```hocon
dependencies {
  nanosvg { git = "github.com/sysl-lang/nanosvg", version = "0.1.0" }
}
```

The module is **`sh.sysl.nanosvg`**; the raw layer is `sh.sysl.nanosvg.c`.

```
sh/sysl/nanosvg/
    nanosvg.sysl        the binding: Image, Shape, Path, Outline, parse, flatten
    tests.sysl          the suite
    c/
        c.sysl          the shim as C declares it, and the numbers its header computes
        shim.c          nanosvg's body, and a read per struct field
        shim.h          what shim.c answers
        nanosvg.h       vendored nanosvg, unmodified
sysl.hocon              who this package is, and what it needs of the machine
LICENSE                 nanosvg's zlib licence
SURVEY.md               every element of nanosvg.h, classified
```

Requires sysl 0.8.6 and `heap`. Links `-lm`.

## What is bound

- **`parse(svg, units = Px, dpi = 96)`** and `parse_bytes(bytes, ...)` give `Result[&Image, Error]`.
  An `Image` is a `&` handle with a `Drop` that frees nanosvg's whole tree.
- **`Image`**: `width`, `height`, `units`, `dpi`, `shapes()`.
- **`Shape`**: `id()` (its own, or its group's), `visible()`, `opacity()`, `fill()`, `stroke()` (`Paint`:
  `None`, `Solid(Color)`, `LinearGradient`, `RadialGradient`), `fill_rule()` (`NonZero` / `EvenOdd`),
  `stroke_width()`, `line_join()`, `line_cap()`, `miter_limit()`, `dashes()`, `dash_offset()`,
  `bounds()`, `paths()`, `flatten(tolerance)`.
- **`Path`**: `closed()`, `segments()`, `points()` (the raw `x, y` floats, a view), `point(i)`,
  `bezier(i)` (four control points), `bounds()`, `flatten(tolerance)`.
- **`Outline`**: the result of flattening: `points`, `closed`, `area()` (signed), `perimeter()`.

A `Shape` or `Path` holds the `&Image` it came from, so the tree outlives every view of it. A
`points()` slice is a view into the tree and lives as long as the `Path` that returned it.

### Flattening

`flatten(tolerance)` splits each cubic at its middle until both control points are within the
tolerance of the chord, so a straight line stays one edge and a tight bend gets many. The guarantee,
proved in the tests by sampling the curve densely and measuring the distance to the polyline, is that
**the curve is never farther than `tolerance` from the polyline**, in the image's units. A closed path
comes back with `closed = true` and **without** its first vertex repeated; an open path keeps its
ends. Zero-length edges are dropped. A tolerance below about one float step of the path's coordinates
is raised to that (nanosvg stores `f32`, so finer asks for nothing but rounding error), and a
tolerance or dpi that is not a positive number is an error.

The holes of a shape are its other paths: `Shape.flatten` returns one `Outline` per path, and
`Shape.fill_rule()` says whether a nested one is a hole (`EvenOdd`) or only when wound the other way
(`NonZero`). The signs of `Outline.area()` show the winding.

### Units

`Units` is `Px`, `Pt`, `Pc`, `Mm`, `Cm`, `In`, and nanosvg converts through `dpi` (default 96):
72 pt, 6 pc, 25.4 mm and 2.54 cm to the inch.

- **`10mm` comes out as 10 in `Units.Mm`, at any dpi**, because the same dpi turns the millimetres
  into pixels and back. A test checks it at 25.4, 72, 96 and 300.
- **Bare numbers are CSS pixels**, as SVG defines, so they scale by `unit / dpi`: 100 unitless units
  are 26.46 mm at 96 dpi and exactly 100 mm at `dpi = 25.4`. **For a file with no units whose numbers
  are already millimetres, pass `Units.Mm` and `25.4`.** An Inkscape file with a `viewBox` and a
  `width="210mm"` needs no thought.
- **nanosvg leaves the image's own `width` and `height` in pixels** whatever unit was asked for (a
  `10mm` image parsed as `Mm` reports 37.8). That is awkward for anything printed, so this binding
  converts them back; the shapes were already right.

## What is absent, and why

See `SURVEY.md` for every element. In short:

- **The rasteriser** (`nanosvgrast.h`): this package is for outlines, not pixels.
- **`nsvgParseFromFile`**: read the file with `sysl.fs` and call `parse_bytes`. It is compiled in and
  never called.
- **`nsvgDuplicatePath`**: it returns a path that no upstream function can free.
- **Gradient detail** (stops, transform, spread): a gradient is reported as `LinearGradient` or
  `RadialGradient` and nothing more is read.
- **Paint order and the gradient ids.**
- **The long list of CSS colour names**: nanosvg ships ten (`red green blue yellow cyan magenta
  black grey gray white`) unless `NANOSVG_ALL_COLOR_KEYWORDS` is defined, which this package leaves
  off; any other name is grey. Hex and `rgb()` are fine.

## nanosvg behaviours worth knowing

All pinned by tests, so a change upstream shows.

- **`visibility:hidden` is ignored**; only `display:none` (attribute, `style`, or on a group) clears
  `visible()`. Hidden shapes are kept in the list and flagged.
- **`fill-opacity` is truncated**, not rounded: 0.5 is alpha 127.
- **A shape with no `fill` attribute is filled black**, as SVG says; `fill='none'` is `Paint.None`.
- **An `<svg` is all the parse requires.** nanosvg returns an empty image for any text at all, so
  `parse` refuses input with no `<svg` as `NotSvg`; a document cut off after it parses to whatever
  was complete.
- **UBSan flags one cast upstream** (see below).

## Vendoring

`c/nanosvg.h` is upstream's `src/nanosvg.h` byte for byte, unmodified:

| | |
|---|---|
| upstream | github.com/memononen/nanosvg, no version tags |
| commit | `239e102ec2c691f2902e20ace2ed36ee4a35cfe6` (2026-07-09) |
| `nanosvg.h` SHA-256 | `e34fd5d084be106cea972d19ce5d27fd96d17ba89f8d06bdceee058420c8b2b0` |
| upstream `LICENSE.txt` SHA-256 | `e82bc3dd03400aecabe12201219ba14750dbc4b36faab58663a7a6068548d372` (zlib; its text is reproduced in `LICENSE`) |

**Does it build on a board?** Only with a libc.

```
clang -c --target=thumbv7m-none-eabi -O2 -I. impl.c          fails: 'string.h' file not found
arm-none-eabi-gcc -c -mcpu=cortex-m33 -O2 -I. impl.c         compiles (newlib)
sysl build-lib . --target thumb-freestanding                 fails the same way
```

The header includes `<string.h>`, `<stdlib.h>`, `<stdio.h>` and `<math.h>` unconditionally, so it is
fine for the Pico SDK, Zephyr and FreeRTOS (all have a libc) and not for a freestanding target. It is
vendored because it is a single header nobody has installed, which is not the same claim as "runs on a
board" and is not made.

**CVE watch.** nanosvg is a hand-written parser of untrusted text, and a vendored copy is not
updated by `brew upgrade`. Re-check upstream at each release.

## Testing

```
sysl test .
SYSL_EXTRA_CFLAGS="-fsanitize=address -g" sysl test .
```

The second reports `instrumented: address (1 binary)`, and because the C is in this tree the flag
compiles nanosvg itself, not only the sysl half.

The oracle is geometry: a circle's area and perimeter against closed form, with a tighter tolerance
giving more points and less error; a triangle, a quadratic arch (two thirds of base times height) and
a three-quarter disc from the SVG 1.1 specification's own path examples worked by hand; and the
flattener's central promise measured against the curve sampled 401 times a segment.

**Fuzzing.** The suite carries a fixed-seed mutation fuzzer over seven documents (3,000 mutants a
run). It was also run at 2,000,000 mutants under `-fsanitize=address,undefined`, and 300,000 under
ASan alone: no memory error. UBSan's `float-cast-overflow` check, left on, stops at the first arc with
a non-finite endpoint:

```
<svg><path d='M1e999 0 A 5 5 0 0 0 5 5'/></svg>
nanosvg.h:2311:10: runtime error: nan is outside the range of representable values of type 'int'
```

`ndivs = (int)(fabsf(da) / (NSVG_PI*0.5f) + 1.0f)` with `da` NaN. Undefined in C, saturating to 0 on
arm64 and harmless there; not patched, and kept as a test input.

## Licence

Two things under two licences: `c/nanosvg.h` is zlib and upstream's (Copyright (c) 2013-14 Mikko
Mononen), everything else is ISC and this repository's. See [LICENSE](LICENSE), which carries
upstream's `LICENSE.txt` verbatim.
