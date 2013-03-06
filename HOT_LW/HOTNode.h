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

#ifndef HOTNODE_H
#define HOTNODE_H

#include <lwpp/node_handler.h>
#include <lwpp/presets.h>
#include <lwpp/vparm.h>

#include <lwpp/point3d.h>
#include <lwpp/vector3d.h>
#include <lwpp/math.h>

#include <lwpp/LW96.h>
#include <lwpp/threads.h>

#include <Ocean.h>

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
		ID_NORM,
		ID_JACB,
		ID_PRES,
	 };

// Map for the Xpanel 
static int nnrm_map[] = { 1, 0};


class HOTNode : public lwpp::XPanelNodeHandler, lwpp::PresetHandler
{

public:

	int nCores;						// number of cores
	lwpp::ThreadGroup thGroup;		// Thread Group

	
	lwpp::LWItem itemID;			// IMPORTANT: To store the LWItemID of the object which the plugin is applied
	
	lwpp::ChannelGroup cgroup;		/// Group for the envelopes

	LWTime newTime;					/// Time from NewTime(to be used in evaluate) 

	
	int			resolution;			/// Resolution of the Grid, 2^r so min 4 (2^4 = 16) to max 16 (2^16 = 2048)
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
	bool		doInterpolation;	/// Interpolate linear or smooth
	bool		doNormals;			/// Calculate the normals for the displacemen, Can't be activated if Chopinees is used
	bool		doJacobian;			/// Jacobian


    lwpp::LWNodeUtility nodeUtil;
	// Node Inputs 
    lwpp::LWNodeInput *inResolution,
					  *inGlobalScale,
					  *inWaveHeight,
					  *inShortestWave,
					  *inChoppiness,
					  *inWindSpeed,
					  *inWindDirection,
					  *inWindAlign,
					  *inDampReflections,
					  *inOceanDepth,
					  *inSeed;
	// Node Outputs
	lwpp::LWNodeOutput	*outDisplacement,
						*outNormal,
						*outFoam,
						*outSpray,
						*outJMinus,
						*outJPlus,
						*EignMinus,
						*EignPlus;

	// Presets management
	lwpp::SimplePreset Preset;

	// Constructor & Destructor
	HOTNode (void *priv, void *context, LWError *err);
	~HOTNode ();
	
	// Asigment operator 
	virtual HOTNode &operator=(const HOTNode &from);
	
	//! Evaluate
	virtual void  Evaluate (LWNodalAccess *na, NodeOutputID outID, NodeValue value);
	
	//! NewTime
	virtual LWError NewTime (LWFrame frame, LWTime time);
	
	//! Interface
	virtual int   Interface (long version, LWInterface *local, void *serverdata);
	virtual void *DataGet (unsigned int vid);
	virtual LWXPRefreshCode DataSet (unsigned int vid, void *value);
	
	
	virtual void  ChangeNotify (LWXPanelID panel, unsigned int cid, unsigned int vid, int event_type);
	
	//! Load
	virtual LWError Load (const lwpp::LoadState &ls);
	
	//! Save
	virtual LWError Save (const lwpp::SaveState &ss);

	// these two replace your existing Load/Save, so you can differentiate between a preset and a normal load if you want to
	virtual LWError LoadSettings(const lwpp::LoadState &ls, bool is_preset = false);
	virtual LWError SaveSettings(const lwpp::SaveState &ss, bool is_preset = false);

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
};



#endif