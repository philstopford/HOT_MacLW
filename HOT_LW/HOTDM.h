/*
======================================================================
Copyright (C) 2010 David Ballesteros
Copyright (C) 2010 Mike Wolf

Houdini Ocean Toolkit ported as a plugin for Lightwave
Started 25 Jan 2010

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

====================================================================== */

#ifndef HOTDM_H
#define HOTDM_H

#include <lwpp/displacement_handler.h>
#include <lwpp/presets.h>

#include <lwpp/objectinfo.h>
#include <lwpp/meshinfo.h>

#include <lwpp/vparm.h>

#include <lwpp/command.h>
#include <lwpp/message.h>

#include <lwpp/point3d.h>
#include <lwpp/vector3d.h>
#include <lwpp/math.h>

#include <string.h>

#include "Ocean.h"

// Xpanel controls enum
enum {	ID_RESO = 0x8001,
		ID_GLSC,
		ID_WAVH,
		ID_SWAV,
		ID_DCHP,
		ID_CHOP,
		ID_WSPD,
		ID_WDIR,
		ID_WALG,
		ID_DREF,
		ID_ODEP,
		ID_SEED,
		ID_INTP,
		ID_PRES
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		// ID_NORM,
		// ID_JACB
	 };

/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
// Map for the Xpanel 
// int nrm_map[] = { 1, 0};

class hotdm : public lwpp::XPanelDisplacementHandler, lwpp::PresetHandler 
{

private:
	// IMPORTANT
	// To store the LWItemID of the object which the plugin is applied
	lwpp::LWItem itemID;

public:

	// lwpp Object Info global
	lwpp::ObjectInfo m_objinf;

	int npoints;

	lwpp::ChannelGroup cgroup;		/// Group for the envelopes

	
	int			resolution;			/// Resolution of the Grid, 2^r so min 4 (2^4 = 16) to max 16 (2^12 = 4096)
	lwpp::VParm	*globalScale;		/// Global scale of the ocean	
	lwpp::VParm	*waveHeight;		/// Aproximate Height of the waves (m)
	lwpp::VParm	*shortestWave;		/// Wavelenght of the shortest wave (m)
	bool doChop;					/// Toggles the application of choppines
	lwpp::VParm	*choppiness;		/// Level of choppines 
	
	lwpp::VParm	*windSpeed;			/// Wind Speed it affects the shape of the waves, units (m/s)
	lwpp::VParm	*windDirection;		/// Direction of the wind, units degrees
	lwpp::VParm	*windAlign;			/// Controls how closely the waves travel in the direction of the wind. Wind alignment.
	
	lwpp::VParm	*dampReflections;	/// Damp reflections - In a “fully developed” ocean you will have waves travelling in both the forward and backwards directions. This parameter damps out the negative direcion waves.
	lwpp::VParm	*oceanDepth;		/// Affects the spectrum of waves generated. Visually in doesn’t seem to have that great an influence.
	
	int			seed;				/// Seed for the random number generator. 
	bool		interpolation;		/// Interpolate linear or smooth
	/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
	// bool		normals;			/// Calculate the normals fro the displacemen, Can't be activated if Chopinees is used
	// bool		jacobian;			/// Jacobian

	// Constructor & Destructor
	hotdm( void *priv, void *context, LWError *err );
	~hotdm();

	// Asigment operator 
	hotdm &operator = ( const hotdm &from );
	
	//! Evaluate
	void Evaluate( lwpp::DisplacementAccess &da );
	
	//! Flags
	unsigned int Flags();

	//! Newtime 
	LWError NewTime( LWFrame fr, LWTime t );

	//! Interface
	int Interface( long version, LWInterface *local, void *serverData );
	void *  DataGet (unsigned int vid);
	LWXPRefreshCode  DataSet (unsigned int vid, void *value);

	//! Load
	LWError Load( const lwpp::LoadState &ls );
	
	//! Save
	LWError Save( const lwpp::SaveState &ss );

	// Presets management
	lwpp::SimplePreset Preset;

	// these two replace your existing Load/Save, so you can differentiate between a preset and a normal load if you want to
	virtual LWError LoadSettings(const lwpp::LoadState &ls, bool is_preset = false);
	virtual LWError SaveSettings(const lwpp::SaveState &ss, bool is_preset = false);

	// [CHANGED]
	//! Load Settings
	//virtual LWError Load(const lwpp::LoadState &ls) {return LoadSettings(ls);}
	//! Save Settings
	//virtual LWError Save(const lwpp::SaveState &ss) {return SaveSettings(ss);}

	virtual LWError LoadPreset(lwpp::LoadState& ls) {return LoadSettings(ls, true);}
	virtual LWError SavePreset(lwpp::SaveState& ss) {return SaveSettings(ss, true);}

protected:
	 // This is where all the wave action takes place
    drw::Ocean        *_ocean;
    drw::OceanContext *_ocean_context;
    float              _ocean_scale;

    // If this is true we will create a new instance of drw::Ocean 
    // next time it runs.
    bool _ocean_needs_rebuild;

}; // end hotdm


#endif