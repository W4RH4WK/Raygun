// The MIT License (MIT)
//
// Copyright (c) 2019,2020 The Raygun Authors.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

void main()
{
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);

    float transition = imageLoad(roughTransitions, pos).r;
    if(transition < 0.001) {
        return;
    }

    vec4 rough = imageLoad(IN_IMAGE, pos);

    vec4 r_1 = imageLoad(IN_IMAGE, pos + OFFSET_1);
    vec4 r_2 = imageLoad(IN_IMAGE, pos + OFFSET_2);

    vec3 col = rough.rgb * (1.f - transition - transition) + r_1.rgb * transition + r_2.rgb * transition;

    imageStore(OUT_IMAGE, pos, vec4(col, rough.a));
}
