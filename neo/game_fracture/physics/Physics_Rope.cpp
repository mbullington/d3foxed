/*=========================================================================
    d3foxed, a modified fork of the fhDOOM engine.
    Copyright (C) 2024 Mark E. Sowden <hogsy@oldtimes-software.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
=========================================================================*/

#include "../../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
#include "Physics_Rope.h"

// Based on the paper here:
// http://www.cs.cmu.edu/afs/cs/academic/class/15462-s13/www/lec_slides/Jakobsen.pdf

CLASS_DECLARATION( idPhysics_Base, ms::Physics_Rope )
END_CLASS

ms::Physics_Rope::Physics_Rope( float length, float mass ) : length( length ), mass( mass )
{
	particles.resize( DEFAULT_NUM_PARTICLES );
}

ms::Physics_Rope::Physics_Rope( idVec3 start, idVec3 end, float length, float mass ) : Physics_Rope( length, mass )
{
	Attach( start, true );
	Attach( end, false );

	//TODO: position the particles between the start and end
}

ms::Physics_Rope::Physics_Rope( idVec3 start, float length, float mass ) : Physics_Rope( length, mass )
{
	Attach( start, true );

	for ( unsigned int i = 1; i < particles.size(); ++i )
	{
		particles[ i ].position    = start;
		particles[ i ].oldPosition = particles[ i ].position;
	}
}

ms::Physics_Rope::~Physics_Rope() = default;

void ms::Physics_Rope::Save( idSaveGame *saveGame ) const
{
	idPhysics_Base::Save( saveGame );

	saveGame->WriteInt( ( int ) particles.size() );
	for ( const auto &i : particles )
	{
		saveGame->WriteVec3( i.position );
		saveGame->WriteVec3( i.oldPosition );
		saveGame->WriteVec3( i.velocity );
		saveGame->WriteBool( i.fixed );
	}
}

void ms::Physics_Rope::Restore( idRestoreGame *restoreGame )
{
	idPhysics_Base::Restore( restoreGame );

	int numSegments;
	restoreGame->ReadInt( numSegments );

	particles.resize( numSegments );
	for ( auto &i : particles )
	{
		restoreGame->ReadVec3( i.position );
		restoreGame->ReadVec3( i.oldPosition );
		restoreGame->ReadVec3( i.velocity );
		restoreGame->ReadBool( i.fixed );
	}
}

void ms::Physics_Rope::WriteToSnapshot( idBitMsgDelta &msg ) const
{
	idPhysics_Base::WriteToSnapshot( msg );
}

void ms::Physics_Rope::ReadFromSnapshot( const idBitMsgDelta &msg )
{
	idPhysics_Base::ReadFromSnapshot( msg );
}

bool ms::Physics_Rope::Evaluate( int timeStepMSec, int endTimeMSec )
{
	float delta = MS2SEC( timeStepMSec ) * 1.0f;

	// add forces
	for ( auto &particle : particles )
	{
		if ( particle.fixed )
		{
			continue;
		}

		particle.velocity = delta * ( gravityVector / 1000.0f ) * mass;

		idVec3 temp        = particle.position;
		idVec3 newPosition = particle.position;

		newPosition += ( particle.position - particle.oldPosition ) + particle.velocity / delta;

		trace_t collision;
		if ( particle.CheckForCollisions( newPosition, collision ) )
		{
			newPosition = collision.endpos;
		}

		particle.position    = newPosition;
		particle.oldPosition = temp;
	}

	// satisfy constraints
	uint numIterations = 2;
	for ( uint i = 0; i < numIterations; ++i )
	{
		for ( uint j = 0; j < particles.size() - 1; ++j )
		{
			Particle &a = particles[ j ];
			Particle &b = particles[ j + 1 ];

			idVec3 deltaVec = a.position - b.position;
			float  deltaLen = deltaVec.Length();
			float  diff     = ( deltaLen > 0 ) ? ( length - deltaLen ) / deltaLen : 0.0f;

			//TODO: defer collision checks to a later stage for opt.

			idVec3 adjust = deltaVec * ( 0.5f * diff );
			if ( !a.fixed )
			{
				idVec3 newPosition = a.position + adjust;
				if ( deltaLen <= length )
				{
					trace_t collision;
					if ( a.CheckForCollisions( newPosition, collision ) )
					{
						newPosition = collision.endpos;
					}
				}

				a.position = newPosition;
			}

			if ( !b.fixed )
			{
				idVec3 newPosition = b.position - adjust;
				if ( deltaLen <= length )
				{
					trace_t collision;
					if ( b.CheckForCollisions( newPosition, collision ) )
					{
						newPosition = collision.endpos;
					}
				}

				b.position = newPosition;
			}
		}
	}

	DebugDraw();

	return true;
}

void ms::Physics_Rope::DebugDraw()
{
	for ( unsigned int i = 0; i < particles.size(); ++i )
	{
		if ( particles[ i ].fixed )
		{
			gameRenderWorld->DebugSphere( idVec4( 1.0f, 0.0f, 0.0f, 1.0f ), idSphere( particles[ i ].position, 0.5f ) );
		}

		if ( i == 0 )
		{
			continue;
		}

		float l = GetAverageSegmentLength();
		float c = ( l == 0 ) ? 0.0f : l / l * 1.0f;
		gameRenderWorld->DebugArrow( idVec4( 0.5f, c, 0.5f, 1.0f ), particles[ i - 1 ].position, particles[ i ].position, 1.0f );
	}
}

/////////////////////////////////////////////////////////////////////////////

bool ms::Physics_Rope::Particle::CheckForCollisions( const idVec3 &newPosition, trace_t &collision ) const
{
	return gameLocal.clip.TracePoint( collision, position, newPosition, MASK_SOLID, nullptr );
}
