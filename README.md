bitblt
======

**bitblt** is a (possibly) faster sprite blitter for [Gamebuino][] with support for gray pixels.

Signature:

```c++
void bitblt( const uint8_t* sprite, int8_t x, int8_t y, bool hide_grey );
```

* **sprite**: the pointer to sprite data generated with **bbltenc**
* **x**: the horizontal screen coordinate
* **y**: the vertial screen coordinate
* **hide_grey**: if ```true```, grey pixels will appear white; if ```false```, grey pixels will appear black

bbltenc
-------

**bbltenc** creates a C byte array with sprite data suitable to use with **bitblt** from a PNG image.

It uses a different format for sprites that takes `2 + width * ( ( height + 7 ) / 8 ) * 2` bytes for an image with `width * height` pixels.

Bugs
----

* **bitblt**: Probably some, if you find one let me know: [@leiradel]
* **bbltenc**: Only handles 32 bpp PNGs with alpha channel

License
-------

The MIT License (MIT)

Copyright (c) 2014 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

[Gamebuino]: http://gamebuino.com/
[@leiradel]: http://twitter.com/leiradel
