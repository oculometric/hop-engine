# Font Serial Specification (`.hfnt`)

```C++
/* 
 * 'Resource' statement loads an asset to be used by the font.
 * - anonymous arguments
 * - children forbidden
 * - identifier required
 */
Resource(
    texture,                                          // resource type, must be 'texture'
    "res://engine/textures/font_IBM_XGA_AI_12x23.png" // path to the asset
) : regular_atlas;

/*
 * 'Atlas' statement defines textures used for each glyph atlas in the font.
 * - named arguments
 * - children forbidden
 * - identifier forbidden
 */
Atlas(
    regular = @regular_atlas, // [REQUIRED] texture to use for text in 'regular' weight, identifier of a previously declared 'Resource' statement
    bold = @regular_atlas, // texture to use for text in 'bold' weight, identifier of a previously declared 'Resource' statement
    glyph_size = [ 14, 25 ] // [REQUIRED] size of each glyph in the texture in pixels, each coordinate must be between 1 and UINT32_MAX inclusive
);
```