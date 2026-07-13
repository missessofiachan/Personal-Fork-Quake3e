/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// q_math.c -- stateless support routines that are included in each code module

// Some of the vector functions are static inline in q_shared.h. q3asm
// doesn't understand static functions though, so we only want them in
// one file. That's what this is about.
#ifdef Q3_VM
#define Q_HAS_SIMD 1
#include <smmintrin.h> // Intel SSE4.1 intrinsics
#include <immintrin.h> // Required for FMA and advanced intrinsics
  // Required for strict integer widths (uint32_t)
#define Q_HAS_SSE4_1 1
#else
#if defined(__SSE4_1__)
#include <smmintrin.h>
#define Q_HAS_SSE4_1 1
#endif
#endif

#include "q_shared.h"

const vec3_t	vec3_origin = {0,0,0};
vec3_t	axisDefault[3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };


vec4_t		colorBlack	= {0, 0, 0, 1};
vec4_t		colorRed	= {1, 0, 0, 1};
vec4_t		colorGreen	= {0, 1, 0, 1};
vec4_t		colorBlue	= {0, 0, 1, 1};
vec4_t		colorYellow	= {1, 1, 0, 1};
vec4_t		colorMagenta= {1, 0, 1, 1};
vec4_t		colorCyan	= {0, 1, 1, 1};
vec4_t		colorWhite	= {1, 1, 1, 1};
vec4_t		colorLtGrey	= {0.75, 0.75, 0.75, 1};
vec4_t		colorMdGrey	= {0.5, 0.5, 0.5, 1};
vec4_t		colorDkGrey	= {0.25, 0.25, 0.25, 1};

// actually there are 35 colors but we want to use bitmask safely
const vec4_t g_color_table[ 64 ] = {

	{0.0f, 0.0f, 0.0f, 1.0f},
	{1.0f, 0.0f, 0.0f, 1.0f},
	{0.0f, 1.0f, 0.0f, 1.0f},
	{1.0f, 1.0f, 0.0f, 1.0f},
	{0.2f, 0.2f, 1.0f, 1.0f}, //{0.0, 0.0, 1.0, 1.0},
	{0.0f, 1.0f, 1.0f, 1.0f},
	{1.0f, 0.0f, 1.0f, 1.0f},
	{1.0f, 1.0f, 1.0f, 1.0f},

	// extended color codes from CPMA/CNQ3:
	{ 1.00000f, 0.50000f, 0.00000f, 1.00000f },	// 8
	{ 0.60000f, 0.60000f, 1.00000f, 1.00000f },	// 9

	// CPMA's alphabet rainbow
	{ 1.00000f, 0.00000f, 0.00000f, 1.00000f },	// a
	{ 1.00000f, 0.26795f, 0.00000f, 1.00000f },	// b
	{ 1.00000f, 0.50000f, 0.00000f, 1.00000f },	// c
	{ 1.00000f, 0.73205f, 0.00000f, 1.00000f },	// d
	{ 1.00000f, 1.00000f, 0.00000f, 1.00000f },	// e
	{ 0.73205f, 1.00000f, 0.00000f, 1.00000f },	// f
	{ 0.50000f, 1.00000f, 0.00000f, 1.00000f },	// g
	{ 0.26795f, 1.00000f, 0.00000f, 1.00000f },	// h
	{ 0.00000f, 1.00000f, 0.00000f, 1.00000f },	// i
	{ 0.00000f, 1.00000f, 0.26795f, 1.00000f },	// j
	{ 0.00000f, 1.00000f, 0.50000f, 1.00000f },	// k
	{ 0.00000f, 1.00000f, 0.73205f, 1.00000f },	// l
	{ 0.00000f, 1.00000f, 1.00000f, 1.00000f },	// m
	{ 0.00000f, 0.73205f, 1.00000f, 1.00000f },	// n
	{ 0.00000f, 0.50000f, 1.00000f, 1.00000f },	// o
	{ 0.00000f, 0.26795f, 1.00000f, 1.00000f },	// p
	{ 0.00000f, 0.00000f, 1.00000f, 1.00000f },	// q
	{ 0.26795f, 0.00000f, 1.00000f, 1.00000f },	// r
	{ 0.50000f, 0.00000f, 1.00000f, 1.00000f },	// s
	{ 0.73205f, 0.00000f, 1.00000f, 1.00000f },	// t
	{ 1.00000f, 0.00000f, 1.00000f, 1.00000f },	// u
	{ 1.00000f, 0.00000f, 0.73205f, 1.00000f },	// v
	{ 1.00000f, 0.00000f, 0.50000f, 1.00000f },	// w
	{ 1.00000f, 0.00000f, 0.26795f, 1.00000f },	// x
	{ 1.0, 1.0, 1.0, 1.0 }, // y, white, duped so all colors can be expressed with this palette
	{ 0.5, 0.5, 0.5, 1.0 }, // z, grey
};


int ColorIndexFromChar( char ccode )
{
	if ( ccode >= '0' && ccode <= '9' ) {
		return ( ccode - '0' );
	}
	else if ( ccode >= 'a' && ccode <= 'z' ) {
		return ( ccode - 'a' + 10 );
	}
	else if ( ccode >= 'A' && ccode <= 'Z' ) {
		return ( ccode - 'A' + 10 );
	}
	else {
		return  ColorIndex( COLOR_WHITE );
	}
}


vec3_t	bytedirs[NUMVERTEXNORMALS] =
{
{-0.525731f, 0.000000f, 0.850651f}, {-0.442863f, 0.238856f, 0.864188f}, 
{-0.295242f, 0.000000f, 0.955423f}, {-0.309017f, 0.500000f, 0.809017f}, 
{-0.162460f, 0.262866f, 0.951056f}, {0.000000f, 0.000000f, 1.000000f}, 
{0.000000f, 0.850651f, 0.525731f}, {-0.147621f, 0.716567f, 0.681718f}, 
{0.147621f, 0.716567f, 0.681718f}, {0.000000f, 0.525731f, 0.850651f}, 
{0.309017f, 0.500000f, 0.809017f}, {0.525731f, 0.000000f, 0.850651f}, 
{0.295242f, 0.000000f, 0.955423f}, {0.442863f, 0.238856f, 0.864188f}, 
{0.162460f, 0.262866f, 0.951056f}, {-0.681718f, 0.147621f, 0.716567f}, 
{-0.809017f, 0.309017f, 0.500000f},{-0.587785f, 0.425325f, 0.688191f}, 
{-0.850651f, 0.525731f, 0.000000f},{-0.864188f, 0.442863f, 0.238856f}, 
{-0.716567f, 0.681718f, 0.147621f},{-0.688191f, 0.587785f, 0.425325f}, 
{-0.500000f, 0.809017f, 0.309017f}, {-0.238856f, 0.864188f, 0.442863f}, 
{-0.425325f, 0.688191f, 0.587785f}, {-0.716567f, 0.681718f, -0.147621f}, 
{-0.500000f, 0.809017f, -0.309017f}, {-0.525731f, 0.850651f, 0.000000f}, 
{0.000000f, 0.850651f, -0.525731f}, {-0.238856f, 0.864188f, -0.442863f}, 
{0.000000f, 0.955423f, -0.295242f}, {-0.262866f, 0.951056f, -0.162460f}, 
{0.000000f, 1.000000f, 0.000000f}, {0.000000f, 0.955423f, 0.295242f}, 
{-0.262866f, 0.951056f, 0.162460f}, {0.238856f, 0.864188f, 0.442863f}, 
{0.262866f, 0.951056f, 0.162460f}, {0.500000f, 0.809017f, 0.309017f}, 
{0.238856f, 0.864188f, -0.442863f},{0.262866f, 0.951056f, -0.162460f}, 
{0.500000f, 0.809017f, -0.309017f},{0.850651f, 0.525731f, 0.000000f}, 
{0.716567f, 0.681718f, 0.147621f}, {0.716567f, 0.681718f, -0.147621f}, 
{0.525731f, 0.850651f, 0.000000f}, {0.425325f, 0.688191f, 0.587785f}, 
{0.864188f, 0.442863f, 0.238856f}, {0.688191f, 0.587785f, 0.425325f}, 
{0.809017f, 0.309017f, 0.500000f}, {0.681718f, 0.147621f, 0.716567f}, 
{0.587785f, 0.425325f, 0.688191f}, {0.955423f, 0.295242f, 0.000000f}, 
{1.000000f, 0.000000f, 0.000000f}, {0.951056f, 0.162460f, 0.262866f}, 
{0.850651f, -0.525731f, 0.000000f},{0.955423f, -0.295242f, 0.000000f}, 
{0.864188f, -0.442863f, 0.238856f}, {0.951056f, -0.162460f, 0.262866f}, 
{0.809017f, -0.309017f, 0.500000f}, {0.681718f, -0.147621f, 0.716567f}, 
{0.850651f, 0.000000f, 0.525731f}, {0.864188f, 0.442863f, -0.238856f}, 
{0.809017f, 0.309017f, -0.500000f}, {0.951056f, 0.162460f, -0.262866f}, 
{0.525731f, 0.000000f, -0.850651f}, {0.681718f, 0.147621f, -0.716567f}, 
{0.681718f, -0.147621f, -0.716567f},{0.850651f, 0.000000f, -0.525731f}, 
{0.809017f, -0.309017f, -0.500000f}, {0.864188f, -0.442863f, -0.238856f}, 
{0.951056f, -0.162460f, -0.262866f}, {0.147621f, 0.716567f, -0.681718f}, 
{0.309017f, 0.500000f, -0.809017f}, {0.425325f, 0.688191f, -0.587785f}, 
{0.442863f, 0.238856f, -0.864188f}, {0.587785f, 0.425325f, -0.688191f}, 
{0.688191f, 0.587785f, -0.425325f}, {-0.147621f, 0.716567f, -0.681718f}, 
{-0.309017f, 0.500000f, -0.809017f}, {0.000000f, 0.525731f, -0.850651f}, 
{-0.525731f, 0.000000f, -0.850651f}, {-0.442863f, 0.238856f, -0.864188f}, 
{-0.295242f, 0.000000f, -0.955423f}, {-0.162460f, 0.262866f, -0.951056f}, 
{0.000000f, 0.000000f, -1.000000f}, {0.295242f, 0.000000f, -0.955423f}, 
{0.162460f, 0.262866f, -0.951056f}, {-0.442863f, -0.238856f, -0.864188f}, 
{-0.309017f, -0.500000f, -0.809017f}, {-0.162460f, -0.262866f, -0.951056f}, 
{0.000000f, -0.850651f, -0.525731f}, {-0.147621f, -0.716567f, -0.681718f}, 
{0.147621f, -0.716567f, -0.681718f}, {0.000000f, -0.525731f, -0.850651f}, 
{0.309017f, -0.500000f, -0.809017f}, {0.442863f, -0.238856f, -0.864188f}, 
{0.162460f, -0.262866f, -0.951056f}, {0.238856f, -0.864188f, -0.442863f}, 
{0.500000f, -0.809017f, -0.309017f}, {0.425325f, -0.688191f, -0.587785f}, 
{0.716567f, -0.681718f, -0.147621f}, {0.688191f, -0.587785f, -0.425325f}, 
{0.587785f, -0.425325f, -0.688191f}, {0.000000f, -0.955423f, -0.295242f}, 
{0.000000f, -1.000000f, 0.000000f}, {0.262866f, -0.951056f, -0.162460f}, 
{0.000000f, -0.850651f, 0.525731f}, {0.000000f, -0.955423f, 0.295242f}, 
{0.238856f, -0.864188f, 0.442863f}, {0.262866f, -0.951056f, 0.162460f}, 
{0.500000f, -0.809017f, 0.309017f}, {0.716567f, -0.681718f, 0.147621f}, 
{0.525731f, -0.850651f, 0.000000f}, {-0.238856f, -0.864188f, -0.442863f}, 
{-0.500000f, -0.809017f, -0.309017f}, {-0.262866f, -0.951056f, -0.162460f}, 
{-0.850651f, -0.525731f, 0.000000f}, {-0.716567f, -0.681718f, -0.147621f}, 
{-0.716567f, -0.681718f, 0.147621f}, {-0.525731f, -0.850651f, 0.000000f}, 
{-0.500000f, -0.809017f, 0.309017f}, {-0.238856f, -0.864188f, 0.442863f}, 
{-0.262866f, -0.951056f, 0.162460f}, {-0.864188f, -0.442863f, 0.238856f}, 
{-0.809017f, -0.309017f, 0.500000f}, {-0.688191f, -0.587785f, 0.425325f}, 
{-0.681718f, -0.147621f, 0.716567f}, {-0.442863f, -0.238856f, 0.864188f}, 
{-0.587785f, -0.425325f, 0.688191f}, {-0.309017f, -0.500000f, 0.809017f}, 
{-0.147621f, -0.716567f, 0.681718f}, {-0.425325f, -0.688191f, 0.587785f}, 
{-0.162460f, -0.262866f, 0.951056f}, {0.442863f, -0.238856f, 0.864188f}, 
{0.162460f, -0.262866f, 0.951056f}, {0.309017f, -0.500000f, 0.809017f}, 
{0.147621f, -0.716567f, 0.681718f}, {0.000000f, -0.525731f, 0.850651f}, 
{0.425325f, -0.688191f, 0.587785f}, {0.587785f, -0.425325f, 0.688191f}, 
{0.688191f, -0.587785f, 0.425325f}, {-0.955423f, 0.295242f, 0.000000f}, 
{-0.951056f, 0.162460f, 0.262866f}, {-1.000000f, 0.000000f, 0.000000f}, 
{-0.850651f, 0.000000f, 0.525731f}, {-0.955423f, -0.295242f, 0.000000f}, 
{-0.951056f, -0.162460f, 0.262866f}, {-0.864188f, 0.442863f, -0.238856f}, 
{-0.951056f, 0.162460f, -0.262866f}, {-0.809017f, 0.309017f, -0.500000f}, 
{-0.864188f, -0.442863f, -0.238856f}, {-0.951056f, -0.162460f, -0.262866f}, 
{-0.809017f, -0.309017f, -0.500000f}, {-0.681718f, 0.147621f, -0.716567f}, 
{-0.681718f, -0.147621f, -0.716567f}, {-0.850651f, 0.000000f, -0.525731f}, 
{-0.688191f, 0.587785f, -0.425325f}, {-0.587785f, 0.425325f, -0.688191f}, 
{-0.425325f, 0.688191f, -0.587785f}, {-0.425325f, -0.688191f, -0.587785f}, 
{-0.587785f, -0.425325f, -0.688191f}, {-0.688191f, -0.587785f, -0.425325f}
};

//==============================================================

int		Q_rand( int *seed ) {
	*seed = (69069 * *seed + 1);
	return *seed;
}

float	Q_random( int *seed ) {
	return ( Q_rand( seed ) & 0xffff ) / (float)0x10000;
}

float	Q_crandom( int *seed ) {
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//=======================================================

signed char ClampChar( int i ) {
	if ( i < -128 ) {
		return -128;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed char ClampCharMove( int i ) {
	if ( i < -127 ) {
		return -127;
	}
	if ( i > 127 ) {
		return 127;
	}
	return i;
}

signed short ClampShort( int i ) {
	if ( i < -32768 ) {
		return -32768;
	}
	if ( i > 0x7fff ) {
		return 0x7fff;
	}
	return i;
}


// this isn't a real cheap function to call!
int DirToByte(vec3_t dir)
{
#if defined(Q_HAS_SSE4_1)
    int i, best;
    float bestd;

    if (!dir)
    {
        return 0;
    }

    bestd = 0.0f;
    best = 0;

    // 1. Broadcast 'dir' components into a SIMD register: [0, dir[2], dir[1], dir[0]]
    __m128 v_dir = _mm_set_ps(0.0f, dir[2], dir[1], dir[0]);

    // Track the best distances in a SIMD register initialized to 0
    __m128 v_bestd = _mm_setzero_ps(); 

    // 2. Process 4 vectors at a time. 
    // Note: 162 isn't perfectly divisible by 4 (162 / 4 = 40.5).
    // We loop up to 160 safely using SIMD, then handle the last 2 scalars at the end.
    for (i = 0; i < 160; i += 4)
    {
        // Load 4 consecutive vec3s from the bytedirs table
        __m128 b0 = _mm_set_ps(0.0f, bytedirs[i+0][2], bytedirs[i+0][1], bytedirs[i+0][0]);
        __m128 b1 = _mm_set_ps(0.0f, bytedirs[i+1][2], bytedirs[i+1][1], bytedirs[i+1][0]);
        __m128 b2 = _mm_set_ps(0.0f, bytedirs[i+2][2], bytedirs[i+2][1], bytedirs[i+2][0]);
        __m128 b3 = _mm_set_ps(0.0f, bytedirs[i+3][2], bytedirs[i+3][1], bytedirs[i+3][0]);

        // Calculate 4 dot products instantly
        // 0x71 mask: multiply X, Y, Z components and store the result in the lowest slot (index 0)
        __m128 dot0 = _mm_dp_ps(v_dir, b0, 0x71);
        __m128 dot1 = _mm_dp_ps(v_dir, b1, 0x71);
        __m128 dot2 = _mm_dp_ps(v_dir, b2, 0x71);
        __m128 dot3 = _mm_dp_ps(v_dir, b3, 0x71);

        // Pack the 4 dot products into a single SIMD register: [dot3, dot2, dot1, dot0]
        __m128 dots = _mm_unpacklo_ps(_mm_unpacklo_ps(dot0, dot2), _mm_unpacklo_ps(dot1, dot3));

        // Scalar extraction out of the packed register to update the best match
        // (Keeping the conditional updates scalar prevents complex mask generation errors)
        float d0 = _mm_cvtss_f32(dots);
        float d1 = _mm_cvtss_f32(_mm_shuffle_ps(dots, dots, _MM_SHUFFLE(1, 1, 1, 1)));
        float d2 = _mm_cvtss_f32(_mm_shuffle_ps(dots, dots, _MM_SHUFFLE(2, 2, 2, 2)));
        float d3 = _mm_cvtss_f32(_mm_shuffle_ps(dots, dots, _MM_SHUFFLE(3, 3, 3, 3)));

        if (d0 > bestd) { bestd = d0; best = i + 0; }
        if (d1 > bestd) { bestd = d1; best = i + 1; }
        if (d2 > bestd) { bestd = d2; best = i + 2; }
        if (d3 > bestd) { bestd = d3; best = i + 3; }
    }

    // 3. Clean up loop remainder (elements 160 and 161)
    for (; i < NUMVERTEXNORMALS; i++)
    {
        float d = dir[0]*bytedirs[i][0] + dir[1]*bytedirs[i][1] + dir[2]*bytedirs[i][2];
        if (d > bestd)
        {
            bestd = d;
            best = i;
        }
    }

    return best;
#else
    int i, best;
    float d, bestd;

    if (!dir)
    {
        return 0;
    }

    bestd = 0.0f;
    best = 0;
    for (i = 0; i < NUMVERTEXNORMALS; i++)
    {
        d = dir[0]*bytedirs[i][0] + dir[1]*bytedirs[i][1] + dir[2]*bytedirs[i][2];
        if (d > bestd)
        {
            bestd = d;
            best = i;
        }
    }

    return best;
#endif
}

void ByteToDir( int b, vec3_t dir ) {
	if ( b < 0 || b >= NUMVERTEXNORMALS ) {
		VectorCopy( vec3_origin, dir );
		return;
	}
	VectorCopy (bytedirs[b], dir);
}


unsigned ColorBytes3 (float r, float g, float b) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;

	return i;
}

unsigned ColorBytes4 (float r, float g, float b, float a) {
	unsigned	i;

	( (byte *)&i )[0] = r * 255;
	( (byte *)&i )[1] = g * 255;
	( (byte *)&i )[2] = b * 255;
	( (byte *)&i )[3] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out ) {
	float	max;
	
	max = in[0];
	if ( in[1] > max ) {
		max = in[1];
	}
	if ( in[2] > max ) {
		max = in[2];
	}

	if ( !max ) {
		VectorClear( out );
	} else {
		out[0] = in[0] / max;
		out[1] = in[1] / max;
		out[2] = in[2] / max;
	}
	return max;
}


/*
=====================
PlaneFromPoints

Returns false if the triangle is degenerate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c ) {
	vec3_t	d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );
	if ( VectorNormalize( plane ) == 0 ) {
		return qfalse;
	}

	plane[3] = DotProduct( a, plane );
	return qtrue;
}

/*
==================
SetupRotationMatrix

Setup rotation matrix given the normalized direction vector and angle to rotate
around this vector. Adapted from Mesa 3D.
==================
*/
void SetupRotationMatrix( vec3_t matrix[3], const vec3_t dir, float degrees ) {
	vec_t	angle, s, c, one_c, xx, yy, zz, xy, yz, zx, xs, ys, zs;

	angle = DEG2RAD(degrees);
	s = sin(angle);
	c = cos(angle);
	one_c = 1.0F - c;

	xx = dir[0] * dir[0];
	yy = dir[1] * dir[1];
	zz = dir[2] * dir[2];
	xy = dir[0] * dir[1];
	yz = dir[1] * dir[2];
	zx = dir[2] * dir[0];
	xs = dir[0] * s;
	ys = dir[1] * s;
	zs = dir[2] * s;

	matrix[0][0] = (one_c * xx) + c;
	matrix[0][1] = (one_c * xy) - zs;
	matrix[0][2] = (one_c * zx) + ys;

	matrix[1][0] = (one_c * xy) + zs;
	matrix[1][1] = (one_c * yy) + c;
	matrix[1][2] = (one_c * yz) - xs;

	matrix[2][0] = (one_c * zx) - ys;
	matrix[2][1] = (one_c * yz) + xs;
	matrix[2][2] = (one_c * zz) + c;
}

/*
===============
RotatePointAroundVector
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point,
							 float degrees ) {
	vec3_t matrix[3];

	SetupRotationMatrix(matrix, dir, degrees);
	VectorRotate(point, matrix, dst);
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[3], float yaw ) {

	// create an arbitrary axis[1] 
	PerpendicularVector( axis[1], axis[0] );

	// rotate it around axis[0] by yaw
	if ( yaw ) {
		vec3_t	temp;

		VectorCopy( axis[1], temp );
		RotatePointAroundVector( axis[1], axis[0], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[0], axis[1], axis[2] );
}



void vectoangles( const vec3_t value1, vec3_t angles ) {
	float	forward;
	float	yaw, pitch;
	
	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	}
	else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}


/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[3] ) {
	vec3_t	right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[0], right, axis[2] );
	VectorSubtract( vec3_origin, right, axis[1] );
}

void AxisClear( vec3_t axis[3] ) {
	axis[0][0] = 1;
	axis[0][1] = 0;
	axis[0][2] = 0;
	axis[1][0] = 0;
	axis[1][1] = 1;
	axis[1][2] = 0;
	axis[2][0] = 0;
	axis[2][1] = 0;
	axis[2][2] = 1;
}

void AxisCopy( vec3_t in[3], vec3_t out[3] ) {
	VectorCopy( in[0], out[0] );
	VectorCopy( in[1], out[1] );
	VectorCopy( in[2], out[2] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t p, const vec3_t normal )
{
	float d;
	vec3_t n;
	float inv_denom;

	inv_denom =  DotProduct( normal, normal );
#ifndef Q3_VM
	assert( Q_fabs(inv_denom) != 0.0f ); // zero vectors get here
#endif
	inv_denom = 1.0f / inv_denom;

	d = DotProduct( normal, p ) * inv_denom;

	n[0] = normal[0] * inv_denom;
	n[1] = normal[1] * inv_denom;
	n[2] = normal[2] * inv_denom;

	dst[0] = p[0] - d * n[0];
	dst[1] = p[1] - d * n[1];
	dst[2] = p[2] - d * n[2];
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up) {
	float		d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct (right, forward);
	VectorMA (right, -d, forward, right);
	VectorNormalize (right);
	CrossProduct (right, forward, up);
}


void VectorRotate( const vec3_t in, const vec3_t matrix[3], vec3_t out )
{
#if defined(Q_HAS_SSE4_1)

    __m128 v_in = _mm_set_ps(0.0f, in[2], in[1], in[0]);

    __m128 row0 = _mm_set_ps(0.0f, matrix[0][2], matrix[0][1], matrix[0][0]);
    __m128 row1 = _mm_set_ps(0.0f, matrix[1][2], matrix[1][1], matrix[1][0]);
    __m128 row2 = _mm_set_ps(0.0f, matrix[2][2], matrix[2][1], matrix[2][0]);

    __m128 dot0 = _mm_dp_ps(v_in, row0, 0x71);
    __m128 dot1 = _mm_dp_ps(v_in, row1, 0x71);
    __m128 dot2 = _mm_dp_ps(v_in, row2, 0x71);

    _mm_store_ss(&out[0], dot0);
    _mm_store_ss(&out[1], dot1);
    _mm_store_ss(&out[2], dot2);
#else
    // QVM FALLBACK (Simple scalar math q3asm understands)
    out[0] = in[0] * matrix[0][0] + in[1] * matrix[0][1] + in[2] * matrix[0][2];
    out[1] = in[0] * matrix[1][0] + in[1] * matrix[1][1] + in[2] * matrix[1][2];
    out[2] = in[0] * matrix[2][0] + in[1] * matrix[2][1] + in[2] * matrix[2][2];
#endif
}

//============================================================================
#ifdef _MSC_SSE2
#include <intrin.h>
#endif

/*
** float Q_rsqrt( float number )
*/
float Q_rsqrt(float number)
{
#ifndef Q3_VM
    // 1. Load the single scalar float into a SIMD register
    __m128 reg = _mm_set_ss(number);

    // 2. Compute the reciprocal square root using specialized hardware silicon.
    // This gives a highly accurate initial approximation instantly.
    reg = _mm_rsqrt_ss(reg);

    float y;
    _mm_store_ss(&y, reg);

    // 3. One iteration of Newton-Raphson refinement to match the precision 
    // of the original Quake III implementation perfectly.
    return y * (1.5f - (number * 0.5f * y * y));
#else
    // Original legendary scalar bit-hack for QVM compilation compatibility
    floatint_t t;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = number * 0.5F;
    t.f = number;
    t.i = 0x5f3759df - (t.i >> 1); // what the fuck?
    y = t.f;
    y = y * (threehalfs - (x2 * y * y));

    return y;
#endif
}


float Q_fabs( float f ) {
	floatint_t fi;
	fi.f = f;
	fi.i &= 0x7FFFFFFF;
	return fi.f;
}


//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle (float from, float to, float frac) {
	float	a;

	if ( to - from > 180 ) {
		to -= 360;
	}
	if ( to - from < -180 ) {
		to += 360;
	}
	a = from + frac * (to - from);

	return a;
}


/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract( float a1, float a2 ) {
	float	a;

	a = a1 - a2;
	while ( a > 180 ) {
		a -= 360;
	}
	while ( a < -180 ) {
		a += 360;
	}
	return a;
}


void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 ) {
	v3[0] = AngleSubtract( v1[0], v2[0] );
	v3[1] = AngleSubtract( v1[1], v2[1] );
	v3[2] = AngleSubtract( v1[2], v2[2] );
}


float	AngleMod(float a) {
	a = (360.0/65536) * ((int)(a*(65536/360.0)) & 65535);
	return a;
}


/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360 ( float angle ) {
	return (360.0 / 65536) * ((int)(angle * (65536 / 360.0)) & 65535);
}


/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180 ( float angle ) {
	angle = AngleNormalize360( angle );
	if ( angle > 180.0 ) {
		angle -= 360.0;
	}
	return angle;
}


/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta ( float angle1, float angle2 ) {
	return AngleNormalize180( angle1 - angle2 );
}


//============================================================


/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits (cplane_t *out) {
	int	bits, j;

	// for fast box on planeside test
	bits = 0;
	for (j=0 ; j<3 ; j++) {
		if (out->normal[j] < 0) {
			bits |= 1<<j;
		}
	}
	out->signbits = bits;
}


/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2
==================
*/
int BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{

	// fast axial cases
	if (p->type < 3)
	{
		if (p->dist <= emins[p->type])
			return 1;
		if (p->dist >= emaxs[p->type])
			return 2;
		return 3;
	}

	#if Q_HAS_SIMD
	if (p->signbits < 8)
	{
		__m128 v_emins = _mm_loadu_ps(emins);
		__m128 v_emaxs = _mm_loadu_ps(emaxs);
		__m128 v_normal = _mm_loadu_ps(p->normal);

		unsigned int m0 = (p->signbits & 1) ? 0xFFFFFFFF : 0;
		unsigned int m1 = (p->signbits & 2) ? 0xFFFFFFFF : 0;
		unsigned int m2 = (p->signbits & 4) ? 0xFFFFFFFF : 0;
		__m128 mask = _mm_set_ps(0.0f, *(float*)&m2, *(float*)&m1, *(float*)&m0);

		__m128 v_dist0 = _mm_or_ps(_mm_and_ps(mask, v_emins), _mm_andnot_ps(mask, v_emaxs));
		__m128 v_dist1 = _mm_or_ps(_mm_and_ps(mask, v_emaxs), _mm_andnot_ps(mask, v_emins));

		__m128 mul0 = _mm_mul_ps(v_normal, v_dist0);
		__m128 mul1 = _mm_mul_ps(v_normal, v_dist1);

		__m128 shuf0_1 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 shuf0_2 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 sum0 = _mm_add_ss(mul0, _mm_add_ss(shuf0_1, shuf0_2));

		__m128 shuf1_1 = _mm_shuffle_ps(mul1, mul1, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 shuf1_2 = _mm_shuffle_ps(mul1, mul1, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 sum1 = _mm_add_ss(mul1, _mm_add_ss(shuf1_1, shuf1_2));

		float dist0, dist1;
		_mm_store_ss(&dist0, sum0);
		_mm_store_ss(&dist1, sum1);

		int sides = 0;
		if (dist0 >= p->dist)
			sides = 1;
		if (dist1 < p->dist)
			sides |= 2;

		return sides;
	}
#endif

// general case
	float	dist[2];
	int		sides, b, i;
	dist[0] = dist[1] = 0;
	if (p->signbits < 8) // >= 8: default case is original code (dist[0]=dist[1]=0)
	{
		for (i=0 ; i<3 ; i++)
		{
			b = (p->signbits >> i) & 1;
			dist[ b] += p->normal[i]*emaxs[i];
			dist[!b] += p->normal[i]*emins[i];
		}
	}
	sides = 0;
	if (dist[0] >= p->dist)
		sides = 1;
	if (dist[1] < p->dist)
		sides |= 2;
	return sides;
}


/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs ) {
	int		i;
	vec3_t	corner;
	float	a, b;

	for (i=0 ; i<3 ; i++) {
		a = fabs( mins[i] );
		b = fabs( maxs[i] );
		corner[i] = a > b ? a : b;
	}

	return VectorLength (corner);
}


void ClearBounds( vec3_t mins, vec3_t maxs ) {
	mins[0] = mins[1] = mins[2] = 99999;
	maxs[0] = maxs[1] = maxs[2] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs ) {
#if Q_HAS_SIMD
	__m128 pt = _mm_loadu_ps(v);
	__m128 mn = _mm_loadu_ps(mins);
	__m128 mx = _mm_loadu_ps(maxs);

	__m128 new_mn = _mm_min_ps(pt, mn);
	__m128 new_mx = _mm_max_ps(pt, mx);

	_mm_store_ss(&mins[0], new_mn);
	_mm_store_ss(&mins[1], _mm_shuffle_ps(new_mn, new_mn, _MM_SHUFFLE(1, 1, 1, 1)));
	_mm_store_ss(&mins[2], _mm_shuffle_ps(new_mn, new_mn, _MM_SHUFFLE(2, 2, 2, 2)));

	_mm_store_ss(&maxs[0], new_mx);
	_mm_store_ss(&maxs[1], _mm_shuffle_ps(new_mx, new_mx, _MM_SHUFFLE(1, 1, 1, 1)));
	_mm_store_ss(&maxs[2], _mm_shuffle_ps(new_mx, new_mx, _MM_SHUFFLE(2, 2, 2, 2)));
#else
	if ( v[0] < mins[0] ) {
		mins[0] = v[0];
	}
	if ( v[0] > maxs[0] ) {
		maxs[0] = v[0];
	}
	if ( v[1] < mins[1] ) {
		mins[1] = v[1];
	}
	if ( v[1] > maxs[1] ) {
		maxs[1] = v[1];
	}
	if ( v[2] < mins[2] ) {
		mins[2] = v[2];
	}
	if ( v[2] > maxs[2] ) {
		maxs[2] = v[2];
	}
#endif
}

qboolean BoundsIntersect(const vec3_t mins, const vec3_t maxs,
		const vec3_t mins2, const vec3_t maxs2)
{
	#ifndef Q3_VM
    // 1. Load the bounds into registers (padding 4th slot with 0)
    __m128 v_mins  = _mm_set_ps(0.0f, mins[2],  mins[1],  mins[0]);
    __m128 v_maxs  = _mm_set_ps(0.0f, maxs[2],  maxs[1],  maxs[0]);
    __m128 v_mins2 = _mm_set_ps(0.0f, mins2[2], mins2[1], mins2[0]);
    __m128 v_maxs2 = _mm_set_ps(0.0f, maxs2[2], maxs2[1], maxs2[0]);

    // 2. Perform simultaneous comparisons across X, Y, and Z axes
    // cmplt: returns 0xFFFFFFFF if true, 0x0 if false per component
    __m128 cmp1 = _mm_cmplt_ps(v_maxs, v_mins2);  // Is maxs < mins2?
    __m128 cmp2 = _mm_cmpgt_ps(v_mins, v_maxs2);  // Is mins > maxs2?

    // 3. Combine the comparison results using a bitwise OR
    __m128 combined = _mm_or_ps(cmp1, cmp2);

    // 4. Movemask extracts the most significant bit of each float slot 
    // into a standard integer (slots 0, 1, 2 correspond to bits 0, 1, 2)
    int mask = _mm_movemask_ps(combined);

    // 5. If any of the lower 3 bits (value 1, 2, or 4) are set, an exclusion condition met.
    // Masking with 7 (binary 0111) checks axes X, Y, and Z simultaneously.
    if (mask & 7)
    {
        return qfalse; 
    }

    return qtrue;
	#else
    // Original scalar fallback for QVM
	if ( maxs[0] < mins2[0] ||
		maxs[1] < mins2[1] ||
		maxs[2] < mins2[2] ||
		mins[0] > maxs2[0] ||
		mins[1] > maxs2[1] ||
		mins[2] > maxs2[2])
	{
		return qfalse;
	}

	return qtrue;
}
#endif
}


qboolean BoundsIntersectSphere(const vec3_t mins, const vec3_t maxs,
		const vec3_t origin, vec_t radius)
		{
#ifndef Q3_VM
    // 1. Load bounds and origin (padding 4th slot with 0)
    __m128 v_mins   = _mm_set_ps(0.0f, mins[2],   mins[1],   mins[0]);
    __m128 v_maxs   = _mm_set_ps(0.0f, maxs[2],   maxs[1],   maxs[0]);
    __m128 v_origin = _mm_set_ps(0.0f, origin[2], origin[1], origin[0]);

    // 2. Broadcast the radius across all slots
    __m128 v_radius = _mm_set1_ps(radius);

    // 3. Expand the sphere origin out into a min/max bounding box
    __m128 sphere_mins = _mm_sub_ps(v_origin, v_radius);
    __m128 sphere_maxs = _mm_add_ps(v_origin, v_radius);

    // 4. Simultaneous check: Is sphere completely outside the AABB?
    __m128 cmp1 = _mm_cmpgt_ps(sphere_mins, v_maxs); // origin - radius > maxs
    __m128 cmp2 = _mm_cmplt_ps(sphere_maxs, v_mins); // origin + radius < mins

    // 5. Extract bitmask of the results
    int mask = _mm_movemask_ps(_mm_or_ps(cmp1, cmp2));

    // If any X, Y, or Z bits (lower 3 bits) are set, there is no intersection
    if (mask & 7)
    {
        return qfalse;
    }

    return qtrue;
#else
{
	if ( origin[0] - radius > maxs[0] ||
		origin[0] + radius < mins[0] ||
		origin[1] - radius > maxs[1] ||
		origin[1] + radius < mins[1] ||
		origin[2] - radius > maxs[2] ||
		origin[2] + radius < mins[2])
	{
		return qfalse;
	}

	return qtrue;
	#endif
}

qboolean BoundsIntersectPoint(const vec3_t mins, const vec3_t maxs,
		const vec3_t origin)
{
	#ifndef Q3_VM
    __m128 v_mins   = _mm_set_ps(0.0f, mins[2],   mins[1],   mins[0]);
    __m128 v_maxs   = _mm_set_ps(0.0f, maxs[2],   maxs[1],   maxs[0]);
    __m128 v_origin = _mm_set_ps(0.0f, origin[2], origin[1], origin[0]);

    // Check if the origin point escapes the bounds on any axis
    __m128 cmp1 = _mm_cmpgt_ps(v_origin, v_maxs); // origin > maxs
    __m128 cmp2 = _mm_cmplt_ps(v_origin, v_mins); // origin < mins

    int mask = _mm_movemask_ps(_mm_or_ps(cmp1, cmp2));

    if (mask & 7)
    {
        return qfalse;
    }

    return qtrue;
	#else
	if ( origin[0] > maxs[0] ||
		origin[0] < mins[0] ||
		origin[1] > maxs[1] ||
		origin[1] < mins[1] ||
		origin[2] > maxs[2] ||
		origin[2] < mins[2])
	{
		return qfalse;
	}

	return qtrue;
	#endif
}

vec_t VectorNormalize(vec3_t v)
{
#ifndef Q3_VM
    // 1. Load the 3 floats (safely padding the 4th element with 0.0f)
    __m128 x = _mm_set_ps(0.0f, v[2], v[1], v[0]);

    // 2. Compute dot product of the vector with itself (X^2 + Y^2 + Z^2)
    // 0x77 mask: Multiply slots 0,1,2 and broadcast the sum to ALL slots of 'sum'
    __m128 sum = _mm_dp_ps(x, x, 0x77);

    float length;
    _mm_store_ss(&length, sum);

    if (length > 0.0f) 
    {
        // 3. Take the square root of the sum
        __m128 sqrt_len = _mm_sqrt_ps(sum);

        // Extract the actual float length to return later
        _mm_store_ss(&length, sqrt_len);

        // 4. Divide the original vector components by the calculated length
        __m128 norm = _mm_div_ps(x, sqrt_len);

        // 5. Store back to memory
        _mm_store_ss(&v[0], norm);
        _mm_store_ss(&v[1], _mm_shuffle_ps(norm, norm, _MM_SHUFFLE(1, 1, 1, 1)));
        _mm_store_ss(&v[2], _mm_shuffle_ps(norm, norm, _MM_SHUFFLE(2, 2, 2, 2)));
    }
    return length;
#else
    // Original scalar fallback for QVM compiler
    float length, ilength;
    length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (length)
    {
        ilength = 1 / (float)sqrt(length);
        length *= ilength;
        v[0] *= ilength;
        v[1] *= ilength;
        v[2] *= ilength;
    }
    return length;
#endif
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out) {
	float	length, ilength;

	length = v[0]*v[0] + v[1]*v[1] + v[2]*v[2];

	if (length)
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
		ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[0] = v[0]*ilength;
		out[1] = v[1]*ilength;
		out[2] = v[2]*ilength;
	} else {
		VectorClear( out );
	}
		
	return length;

}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc) {
	vecc[0] = veca[0] + scale*vecb[0];
	vecc[1] = veca[1] + scale*vecb[1];
	vecc[2] = veca[2] + scale*vecb[2];
}


vec_t _DotProduct( const vec3_t v1, const vec3_t v2 ) {
	return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]-vecb[0];
	out[1] = veca[1]-vecb[1];
	out[2] = veca[2]-vecb[2];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out ) {
	out[0] = veca[0]+vecb[0];
	out[1] = veca[1]+vecb[1];
	out[2] = veca[2]+vecb[2];
}

void _VectorCopy( const vec3_t in, vec3_t out ) {
	out[0] = in[0];
	out[1] = in[1];
	out[2] = in[2];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out ) {
	out[0] = in[0]*scale;
	out[1] = in[1]*scale;
	out[2] = in[2]*scale;
	out[3] = in[3]*scale;
}


int Q_log2( int val ) {
	int answer;

	answer = 0;
	while ( ( val>>=1 ) != 0 ) {
		answer++;
	}
	return answer;
}



/*
=================
PlaneTypeForNormal
=================
*/
/*
int	PlaneTypeForNormal (vec3_t normal) {
	if ( normal[0] == 1.0 )
		return PLANE_X;
	if ( normal[1] == 1.0 )
		return PLANE_Y;
	if ( normal[2] == 1.0 )
		return PLANE_Z;
	
	return PLANE_NON_AXIAL;
}
*/


/*
================
MatrixMultiply
================
*/
void MatrixMultiply(float in1[3][3], float in2[3][3], float out[3][3]) {
	out[0][0] = in1[0][0] * in2[0][0] + in1[0][1] * in2[1][0] +
				in1[0][2] * in2[2][0];
	out[0][1] = in1[0][0] * in2[0][1] + in1[0][1] * in2[1][1] +
				in1[0][2] * in2[2][1];
	out[0][2] = in1[0][0] * in2[0][2] + in1[0][1] * in2[1][2] +
				in1[0][2] * in2[2][2];
	out[1][0] = in1[1][0] * in2[0][0] + in1[1][1] * in2[1][0] +
				in1[1][2] * in2[2][0];
	out[1][1] = in1[1][0] * in2[0][1] + in1[1][1] * in2[1][1] +
				in1[1][2] * in2[2][1];
	out[1][2] = in1[1][0] * in2[0][2] + in1[1][1] * in2[1][2] +
				in1[1][2] * in2[2][2];
	out[2][0] = in1[2][0] * in2[0][0] + in1[2][1] * in2[1][0] +
				in1[2][2] * in2[2][0];
	out[2][1] = in1[2][0] * in2[0][1] + in1[2][1] * in2[1][1] +
				in1[2][2] * in2[2][1];
	out[2][2] = in1[2][0] * in2[0][2] + in1[2][1] * in2[1][2] +
				in1[2][2] * in2[2][2];
}


void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up) {
	float		angle;
	static float		sr, sp, sy, cr, cp, cy;
	// static to help MS compiler fp bugs

	angle = angles[YAW] * (M_PI*2 / 360);
	sy = sin(angle);
	cy = cos(angle);
	angle = angles[PITCH] * (M_PI*2 / 360);
	sp = sin(angle);
	cp = cos(angle);
	angle = angles[ROLL] * (M_PI*2 / 360);
	sr = sin(angle);
	cr = cos(angle);

	if (forward)
	{
		forward[0] = cp*cy;
		forward[1] = cp*sy;
		forward[2] = -sp;
	}
	if (right)
	{
		right[0] = (-1*sr*sp*cy+-1*cr*-sy);
		right[1] = (-1*sr*sp*sy+-1*cr*cy);
		right[2] = -1*sr*cp;
	}
	if (up)
	{
		up[0] = (cr*sp*cy+-sr*-sy);
		up[1] = (cr*sp*sy+-sr*cy);
		up[2] = cr*cp;
	}
}

/*
** assumes "src" is normalized
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int	pos;
	int i;
	float minelem = 1.0F;
	vec3_t tempvec;

	/*
	** find the smallest magnitude axially aligned vector
	*/
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( fabs( src[i] ) < minelem )
		{
			pos = i;
			minelem = fabs( src[i] );
		}
	}
	tempvec[0] = tempvec[1] = tempvec[2] = 0.0F;
	tempvec[pos] = 1.0F;

	/*
	** project the point onto the plane defined by src
	*/
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	** normalize the result
	*/
	VectorNormalize( dst );
}


/*
================
Q_isnan

Don't pass doubles to this
================
*/
int Q_isnan( float x )
{
	floatint_t fi;

	fi.f = x;
	fi.u &= 0x7FFFFFFF;
	fi.u = 0x7F800000 - fi.u;

	return (int)( fi.u >> 31 );
}
//------------------------------------------------------------------------


/*
================
Q_isfinite
================
*/
static int Q_isfinite( float f )
{
	floatint_t fi;
	fi.f = f;

	if ( fi.u == 0xFF800000 || fi.u == 0x7F800000 )
		return 0; // -INF or +INF

	fi.u = 0x7F800000 - (fi.u & 0x7FFFFFFF);
	if ( (int)( fi.u >> 31 ) )
		return 0; // -NAN or +NAN

	return 1;
}


/*
================
Q_atof
================
*/
float Q_atof( const char *str )
{
	float f;

	f = atof( str );

	// modern C11-like implementations of atof() may return INF or NAN
	// which breaks all FP code where such values getting passed
	// and effectively corrupts range checks for cvars as well
	if ( !Q_isfinite( f ) )
		return 0.0f;

	return f;
}


/*
================
Q_log2f
================
*/
float Q_log2f( float f )
{
	const float v = logf( f );
	return v / M_LN2;
}


/*
================
Q_exp2f
================
*/
float Q_exp2f( float f )
{
	return powf( 2.0f, f );
}


#ifndef Q3_VM
/*
=====================
Q_acos

the msvc acos doesn't always return a value between -PI and PI:

int i;
i = 1065353246;
acos(*(float*) &i) == -1.#IND0

=====================
*/
float Q_acos(float c) {
	float angle;

	angle = acos(c);

	if (angle > M_PI) {
		return (float)M_PI;
	}
	if (angle < -M_PI) {
		return (float)M_PI;
	}
	return angle;
}
#endif
