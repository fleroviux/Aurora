#!/bin/sh

glslangValidator -S vert -V100 --vn triangle_vert -o triangle.vert.h triangle.vert.glsl
glslangValidator -S frag -V100 --vn triangle_frag -o triangle.frag.h triangle.frag.glsl

glslangValidator -S vert -V100 --vn output_vert -o output.vert.h output.vert.glsl
glslangValidator -S frag -V100 --vn output_frag -o output.frag.h output.frag.glsl

glslangValidator -S vert -V100 --vn surface_vert -o surface.vert.h surface.vert.glsl
glslangValidator -S frag -V100 --vn surface_frag -o surface.frag.h surface.frag.glsl
