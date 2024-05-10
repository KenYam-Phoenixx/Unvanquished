/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_marks.c -- wall marks

#include "cg_local.h"

/*
===================================================================

MARK POLYS

===================================================================
*/

markPoly_t cg_activeMarkPolys; // double linked list
markPoly_t *cg_freeMarkPolys; // single linked list
markPoly_t cg_markPolys[ MAX_MARK_POLYS ];

/*
===================
CG_InitMarkPolys

This is called at startup and for tournament restarts
===================
*/
void CG_InitMarkPolys()
{
	int i;

	memset( cg_markPolys, 0, sizeof( cg_markPolys ) );

	cg_activeMarkPolys.nextMark = &cg_activeMarkPolys;
	cg_activeMarkPolys.prevMark = &cg_activeMarkPolys;
	cg_freeMarkPolys = cg_markPolys;

	for ( i = 0; i < MAX_MARK_POLYS - 1; i++ )
	{
		cg_markPolys[ i ].nextMark = &cg_markPolys[ i + 1 ];
	}
}

/*
==================
CG_FreeMarkPoly
==================
*/
static void CG_FreeMarkPoly( markPoly_t *le )
{
	if ( !le->prevMark )
	{
		Sys::Drop( "CG_FreeMarkPoly: not active" );
	}

	// remove from the doubly linked active list
	le->prevMark->nextMark = le->nextMark;
	le->nextMark->prevMark = le->prevMark;

	// the free list is only singly linked
	le->nextMark = cg_freeMarkPolys;
	cg_freeMarkPolys = le;
}

/*
===================
CG_AllocMark

Will always succeed, even if it requires freeing an old active mark
===================
*/
static markPoly_t *CG_AllocMarkPoly()
{
	markPoly_t *le;
	int        time;

	if ( !cg_freeMarkPolys )
	{
		// no free entities, so free the one at the end of the chain
		// remove the oldest active entity
		time = cg_activeMarkPolys.prevMark->time;

		while ( cg_activeMarkPolys.prevMark && time == cg_activeMarkPolys.prevMark->time )
		{
			CG_FreeMarkPoly( cg_activeMarkPolys.prevMark );
		}
	}

	le = cg_freeMarkPolys;
	cg_freeMarkPolys = cg_freeMarkPolys->nextMark;

	memset( le, 0, sizeof( *le ) );

	// link into the active list
	le->nextMark = cg_activeMarkPolys.nextMark;
	le->prevMark = &cg_activeMarkPolys;
	cg_activeMarkPolys.nextMark->prevMark = le;
	cg_activeMarkPolys.nextMark = le;
	return le;
}

struct mark_t
{
	// Set at register time.

	qhandle_t shader;
	// origin should be a point within a unit of the plane.
	vec3_t origin;
	// dir should be the plane normal
	vec3_t dir;
	float orientation;
	float red;
	float green;
	float blue;
	float alpha;
	bool alphaFade;
	float radius;
	/* temporary marks will not be stored or randomly oriented,
	but immediately passed to the renderer. */
	bool temporary;

	// Set at processing time.

	vec3_t axis[ 3 ];
	float texCoordScale;
};

std::array<mark_t, MAX_MARK_FRAGMENTS> marks;
size_t numMarks = 0;

void CG_ProcessMarks()
{
	if ( !numMarks )
	{
		return;
	}

	// Shared with the engine.
	static std::vector<markMsgInput_t> markMsgInput;
	static std::vector<markMsgOutput_t> markMsgOutput;

	markMsgInput.reserve( MAX_MARK_FRAGMENTS );
	markMsgOutput.reserve( MAX_MARK_FRAGMENTS );

	markMsgInput.resize( numMarks );
	markMsgOutput.resize( numMarks );

	for ( size_t n = 0; n < numMarks; n++ )
	{
		markMsgInput_t& input = markMsgInput[ n ];
		markOriginalPoints_t& originalPoints = input.originalPoints;
		markProjection_t& projection = input.projection;

		mark_t& m = marks[ n ];

		// create the texture axis
		VectorNormalize2( m.dir, m.axis[ 0 ] );
		PerpendicularVector( m.axis[ 1 ], m.axis[ 0 ] );
		RotatePointAroundVector( m.axis[ 2 ], m.axis[ 0 ], m.axis[ 1 ], m.orientation );
		CrossProduct( m.axis[ 0 ], m.axis[ 2 ], m.axis[ 1 ] );

		m.texCoordScale = 0.5 * 1.0 / m.radius;

		// create the full polygon
		for ( size_t i = 0; i < 3; i++ )
		{
			originalPoints[ 0 ][ i ] = m.origin[ i ] - m.radius * m.axis[ 1 ][ i ] - m.radius * m.axis[ 2 ][ i ];
			originalPoints[ 1 ][ i ] = m.origin[ i ] + m.radius * m.axis[ 1 ][ i ] - m.radius * m.axis[ 2 ][ i ];
			originalPoints[ 2 ][ i ] = m.origin[ i ] + m.radius * m.axis[ 1 ][ i ] + m.radius * m.axis[ 2 ][ i ];
			originalPoints[ 3 ][ i ] = m.origin[ i ] - m.radius * m.axis[ 1 ][ i ] + m.radius * m.axis[ 2 ][ i ];
		}

		VectorScale( m.dir, -20, projection );
	}

	trap_CM_BatchMarkFragments( markMsgInput, markMsgOutput );

	for ( size_t n = 0; n < numMarks; n++ )
	{
		const markMsgOutput_t& output = markMsgOutput[ n ];
		const markFragments_t& markFragments = output.markFragments;
		const markPoints_t& markPoints = output.markPoints;

		mark_t& m = marks[ n ];

		byte colors[ 4 ];
		colors[ 0 ] = m.red * 255;
		colors[ 1 ] = m.green * 255;
		colors[ 2 ] = m.blue * 255;
		colors[ 3 ] = m.alpha * 255;

		for ( int i = 0; i < output.numFragments; i++ )
		{
			const markFragment_t& markFragment = markFragments[ i ];

			// we have an upper limit on the complexity of polygons
			// that we store persistently
			const int numPoints = std::min( markFragment.numPoints, MAX_VERTS_ON_POLY );

			polyVert_t verts[ MAX_VERTS_ON_POLY ];

			for ( int j = 0; j < numPoints; j++ )
			{
				polyVert_t& vert = verts[ j ];
				const markPoint_t& markPoint = markPoints[ markFragment.firstPoint + j ];

				VectorCopy( markPoint, vert.xyz );

				vec3_t delta;
				VectorSubtract( vert.xyz, m.origin, delta );

				vert.st[ 0 ] = 0.5 + DotProduct( delta, m.axis[ 1 ] ) * m.texCoordScale;
				vert.st[ 1 ] = 0.5 + DotProduct( delta, m.axis[ 2 ] ) * m.texCoordScale;
				*(int*) vert.modulate = *(int*) colors;
			}

			// if it is a temporary (shadow) mark, add it immediately and forget about it
			if ( m.temporary )
			{
				trap_R_AddPolyToScene( m.shader, numPoints, verts );
				continue;
			}

			// otherwise save it persistently
			markPoly_t *mp = CG_AllocMarkPoly();

			mp->time = cg.time;
			mp->alphaFade = m.alphaFade;
			mp->shader = m.shader;
			mp->poly.numVerts = numPoints;
			mp->color[ 0 ] = m.red;
			mp->color[ 1 ] = m.green;
			mp->color[ 2 ] = m.blue;
			mp->color[ 3 ] = m.alpha;

			memcpy( mp->verts, verts, numPoints * sizeof( polyVert_t ) );
		}
	}
}

void CG_RegisterMark( qhandle_t shader, const vec3_t origin, const vec3_t dir,
                    float orientation, float red, float green, float blue, float alpha,
                    bool alphaFade, float radius, bool temporary )
{
	if ( !cg_addMarks.Get() )
	{
		return;
	}

	if( temporary )
	{
		if( CG_CullPointAndRadius( origin, M_SQRT2 * radius ) )
		{
			return;
		}
	}

	if ( radius <= 0 )
	{
		Sys::Drop( "CG_ProcessMark called with <= 0 radius" );
	}

	mark_t m;

	m.shader = shader;
	VectorCopy( origin, m.origin );
	VectorCopy( dir, m.dir );
	m.orientation = orientation;
	m.red = red;
	m.green = green;
	m.blue = blue;
	m.alpha = alpha;
	m.alphaFade = alphaFade;
	m.radius = radius;
	m.temporary = temporary;

	marks[ numMarks ] = m;
	numMarks++;
}

void CG_ResetMarks()
{
	numMarks = 0;
}

/*
===============
CG_AddMarks
===============
*/
#define MARK_TOTAL_TIME 10000
#define MARK_FADE_TIME  1000

void CG_AddMarkPolys()
{
	int        j;
	markPoly_t *mp, *next;
	int        t;
	int        fade;

	if ( !cg_addMarks.Get() )
	{
		return;
	}

	mp = cg_activeMarkPolys.nextMark;

	for ( ; mp != &cg_activeMarkPolys; mp = next )
	{
		// grab next now, so if the local entity is freed we
		// still have it
		next = mp->nextMark;

		// see if it is time to completely remove it
		if ( cg.time > mp->time + MARK_TOTAL_TIME )
		{
			CG_FreeMarkPoly( mp );
			continue;
		}

		// fade all marks out with time
		t = mp->time + MARK_TOTAL_TIME - cg.time;

		if ( t < MARK_FADE_TIME )
		{
			fade = 255 * t / MARK_FADE_TIME;

			if ( mp->alphaFade )
			{
				for ( j = 0; j < mp->poly.numVerts; j++ )
				{
					mp->verts[ j ].modulate[ 3 ] = fade;
				}
			}
			else
			{
				for ( j = 0; j < mp->poly.numVerts; j++ )
				{
					mp->verts[ j ].modulate[ 0 ] = mp->color[ 0 ] * fade;
					mp->verts[ j ].modulate[ 1 ] = mp->color[ 1 ] * fade;
					mp->verts[ j ].modulate[ 2 ] = mp->color[ 2 ] * fade;
				}
			}
		}
		trap_R_AddPolyToScene( mp->shader, mp->poly.numVerts, mp->verts );
	}
}
