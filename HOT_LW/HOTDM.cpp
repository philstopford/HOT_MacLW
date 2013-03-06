#include "HOTDM.h"

// Constructor
hotdm::hotdm( void *priv, void *context, LWError *err ) : 
			  lwpp::XPanelDisplacementHandler(priv, context, err),
			  // LWDISPLACEMENT_HCLASS also defines the directory, if you want this shared across different types of plugins, use a common string here
			  Preset(LWDISPLACEMENT_HCLASS, "HOT Ocean Toolkit Displacement Plugin"),
			  resolution(8),
			  doChop(true),
			  seed(0),
			  interpolation(true)
			  // normals(false), /* This DOESNT work for displacement nodes, But left the code to see how other controls can be done */
			  // jacobian(false) /* This DOESNT work for displacement nodes, But left the code to see how other controls can be done */
		
{ 
	if ( (lwpp::LightWave::Major() < 9) ||
		 ( (lwpp::LightWave::Major() == 9) && (lwpp::LightWave::Minor() < 6) ) )
	{
		*err = "This plugin needs a newer version of LightWave 3D (9.6 as a minimum)";
		return;
	}

	// Store the LWItemID of the object which the plugin is applied
	itemID.SetID( (LWItemID) context);

	//Get the group for the envelopes from the item where we apply the plugin
	// In the node version this is not needed, the nodes provide with the handy chanGrp() function
	cgroup = itemID.ChanGroup();

	// Initialize the parameters of the plugin
	// This are variant type of LWVP_DIST because is measured in meters
	globalScale = new lwpp::VParm(LWVP_DIST, "globalScale", "HOT Ocean Toolkit", cgroup.getID(), 50.0);	
	waveHeight = new lwpp::VParm(LWVP_DIST, "waveHeight", "HOT Ocean Toolkit", cgroup.getID(), 1.6);
	shortestWave = new lwpp::VParm(LWVP_DIST, "shortestWave", "HOT Ocean Toolkit", cgroup.getID(), 0.02);
	choppiness = new lwpp::VParm(LWVP_DIST, "choppiness", "HOT Ocean Toolkit", cgroup.getID(), 1.78);
	windSpeed = new lwpp::VParm(LWVP_DIST, "windSpeed", "HOT Ocean Toolkit", cgroup.getID(), 30.0);
	
	// This is a variant type of LWVP_ANGLE because is measured in degrees
	windDirection = new lwpp::VParm(LWVP_ANGLE, "windDirection", "HOT Ocean Toolkit", cgroup.getID(), 0.0);
	
	// This is a variant type of LWVP_FLOAT because it lacks of units
	windAlign = new lwpp::VParm(LWVP_FLOAT, "windAlign", "HOT Ocean Toolkit", cgroup.getID(), 2.0);
	dampReflections = new lwpp::VParm(LWVP_FLOAT, "dampReflections", "HOT Ocean Toolkit", cgroup.getID(), 0.5);

	// This is a variant type of LWVP_DIST because is measured in meters
	oceanDepth = new lwpp::VParm(LWVP_DIST, "oceanDepth", "HOT Ocean Toolkit", cgroup.getID(), 200.0);

	// Get the object info for the item which the plugin is applied
	m_objinf = new lwpp::ObjectInfo(itemID.GetID());

	// Ocean.h variables
	_ocean_needs_rebuild = true;
	_ocean = 0;
    _ocean_context = 0;
    _ocean_scale = 1.0f;

	// Creates a handler for presets from our class
	// We do this the last thing after everything else has been created.
	Preset.SetHandler(this);

	UNUSED(priv);
	UNUSED(err);

}

// Destructor
hotdm::~hotdm()
{
	// Ocean.h variables
	if (_ocean)			delete _ocean;
    if (_ocean_context) delete _ocean_context;

	// Envelopes
	if(globalScale)		delete globalScale;
	if(waveHeight)		delete waveHeight;
	if(shortestWave)	delete shortestWave;
	if(choppiness)		delete choppiness;
	if(windSpeed)		delete windSpeed;
	if(windDirection)	delete windDirection;
	if(windAlign)		delete windAlign;
	if(dampReflections)	delete dampReflections;
	if(oceanDepth)		delete oceanDepth;
}

// Asigment operator 
hotdm &hotdm::operator=(const hotdm &from)
{
	// Parameters with envelopes need to be copied manually
	if (&from != this)
	{
		*globalScale = *from.globalScale;
		*waveHeight = *from.waveHeight;
		*shortestWave = *from.shortestWave;
		*choppiness = *from.choppiness;
		*windSpeed = *from.windSpeed;
		*windDirection = *from.windDirection;
		*windAlign = *from.windAlign;
		*dampReflections = *from.dampReflections;
		*oceanDepth = *from.oceanDepth;
	}
	return *this;
	
}

/* 
======================================================================
Evaluate()
====================================================================== */
void  hotdm::Evaluate( lwpp::DisplacementAccess &da )
{

	// Get the position of the point
	lwpp::Point3d point(da.getSource());
	

	// do the waves
	if (interpolation)
	{
		_ocean_context->eval2_xz( (float) point.x, (float) point.z);
	} else 
	{
		_ocean_context->eval_xz( (float) point.x, (float) point.z);
	}
	
	lwpp::Point3d tmpPoint(point);
	
	if (doChop) 
    {
		tmpPoint.x += _ocean_context->disp[0];
		tmpPoint.y += _ocean_context->disp[1];
		tmpPoint.z += _ocean_context->disp[2];
    }
    else
    {
		tmpPoint.y += _ocean_context->disp[1];
    }

	// The normals are not affected on a displacement map.
	/* This DOESNT work for displacement nodes, But left the code to see how other controls can be done */
	/*if (normals)
    {
		lwpp::Vector3d pnormal( _ocean_context->normal[0],
								_ocean_context->normal[1],
								_ocean_context->normal[2]);
		pnormal.Normalize();
		da.setAvgNormal(pnormal);
    }*/

	// Displace the point
	da.setSource(tmpPoint);
	
}

/* ======================================================================
Flags()
====================================================================== */
unsigned int hotdm::Flags()
{
	// We ask for the World Coordinates of the points, and the average normals.
	// return (LWDMF_BEFOREBONES & LWDMF_NEED_NORMALS);
	// return (LWDMF_AFTERMORPH & LWDMF_NEED_NORMALS);
	return (LWDMF_WORLD & LWDMF_NEED_NORMALS);
}



/* ======================================================================
NewTime()
====================================================================== */
LWError hotdm::NewTime( LWFrame frame, LWTime time ) 
{ 

	// Temporal variables to avoid multiple calls to GetValue
	float stepsize, wHeight, wSpeed, sWave, chpnss, wDir, dRef, wAlign, oDepth ;

	// Update the envelopes
	// WARNING: This needs to be done or the parameters will not work
	globalScale->NewTime(time);
	waveHeight->NewTime(time);
	shortestWave->NewTime(time);
	choppiness->NewTime(time);
	windSpeed->NewTime(time);
	windDirection->NewTime(time);
	windAlign->NewTime(time);
	dampReflections->NewTime(time);
	oceanDepth->NewTime(time);

	// Get the enveloped values
	int gridres  = 1 << int(resolution);
	stepsize = globalScale->GetValuef()/ (float) gridres;
	wHeight = waveHeight->GetValuef();
	wSpeed = windSpeed->GetValuef();
	sWave = shortestWave->GetValuef();
	chpnss = choppiness->GetValuef();
	// Ocean.h expect the win direction in radians
	wDir = windDirection->GetValuef();
	dRef = 1.0f - dampReflections->GetValuef();
	wAlign = windAlign->GetValuef();
	oDepth = oceanDepth->GetValuef();

	// if we need to (re)initialize the ocean, do this
	if (!_ocean || _ocean_needs_rebuild)
    {
        if (_ocean) delete _ocean;
        if (_ocean_context) delete _ocean_context;

		_ocean = new drw::Ocean(	gridres,	// M in Ocean.h
									gridres,	// N in Ocean.h
									stepsize,	// dx in Ocean.h
									stepsize,	// dz in Ocean.h
									wSpeed,		// V in Ocean.h
									sWave,		// l in Ocean.h
									wHeight,	// A in Ocean.h
									wDir,		// w in Ocean.h
									dRef,		// damp in Ocean.h
									wAlign,		// alignment in Ocean.h
									oDepth,		// depth in Ocean.h
									seed);		// seed in Ocean.h
        
		_ocean_scale   = _ocean->get_height_normalize_factor();

		_ocean_context = _ocean->new_context(true,doChop,false,false);
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		// _ocean_context = _ocean->new_context(true,doChop,normals,jacobian);

        _ocean_needs_rebuild = true;
    
		

    }
	
	// sum up the waves at this timestep
    _ocean->update( (float) time,
					*_ocean_context,
					true,
					doChop,
					false,
					false,
                    _ocean_scale * (float) wHeight,
					chpnss);

	/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
	// sum up the waves at this timestep
    /*_ocean->update( (float) time,
					*_ocean_context,
					true,
					doChop,
					normals,
					jacobian,
                    _ocean_scale * (float) wHeight,
					chpnss);*/

	// Transform from radian to degree to print it
	float wDirdeg = lwpp::Degrees<float>(wDir);
	lwpp::SendCommand("StatusMsg", "Resolution: %i | Wav. Height: %.4f | Shrt. Wave: %.4f | Choppiness: %.4f | Wind [ Speed: %.2f | Dir: %.2f | Align: %.2f ] | Damp Ref.: %.2f | O. Depth: %.2f | Seed: %i", gridres, wHeight, sWave, chpnss, wSpeed, wDirdeg, wAlign, dRef, oDepth, seed);

	UNUSED(frame);
	return NULL;
}


/* ======================================================================
Interface()
====================================================================== */
int hotdm::Interface( long version, LWInterface *local, void *serverData )
{
	UNUSED(version);
	UNUSED(serverData);
	
	static LWXPanelControl ctrl[] =
	{
		{ID_RESO, "Resolution",					"iSliderText"},
		{ID_GLSC, "Global Scale",				"distance-env"},
		{ID_WAVH, "Wave Height",				"distance-env"},
		{ID_SWAV, "Shortest Wave",				"distance-env"},
		{ID_DCHP, "Use Choppiness",				"iBoolean"},
		{ID_CHOP, "Choppiness",					"distance-env"},
		{ID_WSPD, "Wind Speed (m/s)",			"float-env"},
		{ID_WDIR, "Wind Direction (º)",			"angle-env"},
		{ID_WALG, "Wind Align",					"float-env"},
		{ID_DREF, "Damp Reflections",			"float-env"},
		{ID_ODEP, "Ocean Depth",				"distance-env"},
		{ID_SEED, "Seed",						"integer"},
		{ID_INTP, "Catmull-Rom Interpolation",	"iBoolean"},
		{ID_PRES, "Presets",					"vPopCmd"},
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		// {ID_NORM, "Do Normals",					"iBoolean"},
		// {ID_JACB, "Jacobian",	"iBoolean"},
		{ 0 }
	};
  
	static LWXPanelDataDesc desc[] =
	{
		{ID_RESO, "Resolution",					"integer"},
		{ID_GLSC, "Global Scale",				"distance-env"},
		{ID_WAVH, "Wave Height",				"distance-env"},
		{ID_SWAV, "Shortest Wave",				"distance-env"},
		{ID_DCHP, "Use Choppiness",				"integer"},
		{ID_CHOP, "Choppiness",					"distance-env"},
		{ID_WSPD, "Wind Speed (m/s)",			"float-env"},
		{ID_WDIR, "Wind Direction (º)",			"angle-env"},
		{ID_WALG, "Wind Align",					"float-env"},
		{ID_DREF, "Damp Reflections",			"float-env"},
		{ID_ODEP, "Ocean Depth",				"distance-env"},
		{ID_SEED, "Seed",						"integer"},
		{ID_INTP, "Catmull-Rom Interpolation",	"integer"},
		{ID_PRES, "Presets",					"integer"},
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		// {ID_NORM, "Do Normals",					"integer"},
		// {ID_JACB, "Jacobian",					"integer"},
		{ 0 }
	};
	
	static LWXPanelHint hints[] = {
		XpLABEL( 0, "HOT Ocean Toolkit" ),
		// Enable Use Choppiness and  Chopinnes controls
		XpENABLE_(ID_DCHP),XpH(ID_CHOP),XpEND,
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		// XpENABLE_MAP_(ID_DCHP, nrm_map), XpH(ID_NORM),XpEND,
		
		XpDIVADD( ID_GLSC ), //Separator after Global Scale
		XpDIVADD( ID_SWAV ),
		XpDIVADD( ID_CHOP ),
		XpDIVADD( ID_WALG ),
		XpDIVADD( ID_ODEP ),
		XpDIVADD( ID_INTP ),

		XpMIN( ID_RESO, 1 ), // Resolution
		XpMAX( ID_RESO, 12 ),
		XpSTEP( ID_RESO, 1 ),

		XpMIN( ID_GLSC, 0 ), // Global Scale 
		XpMAX( ID_GLSC, 5000000 ),
		XpSTEP( ID_GLSC, 1 ),

		XpMIN( ID_WAVH, 0 ), // Wave Height
		XpMAX( ID_WAVH, 5000 ),
		XpSTEP( ID_WAVH, 1 ),
		
		XpMIN( ID_SWAV, 0 ), // Shortest Wave
		XpMAX( ID_SWAV, 500 ),
		XpSTEP( ID_SWAV, 1 ),

		XpMIN( ID_CHOP, 0 ), // Choppiness
		XpMAX( ID_CHOP, 50 ),
		XpSTEP( ID_CHOP, 1 ),

		XpMIN( ID_WSPD, 0 ), // Wind Speed
		XpMAX( ID_WSPD, 50000 ),
		XpSTEP( ID_WSPD, 1 ),

		XpMIN( ID_WALG, 0 ), // Wind Align
		XpMAX( ID_WALG, 10 ),
		XpSTEP( ID_WALG, 1 ),

		XpMIN( ID_DREF, 0 ), // Damp Reflections
		XpMAX( ID_DREF, 1 ),
		XpSTEP( ID_DREF, 1 ),

		XpMIN( ID_ODEP, 0 ), // Ocean Depth
		XpMAX( ID_ODEP, 50000 ),
		XpSTEP( ID_ODEP, 1 ),
 
		XpEND
	}; // end hint[]

	CreateViewXPanel (ctrl, desc, hints);
	Preset.Setup(LW_XPanel, ID_PRES);
	local->panel = LW_XPanel.getID();

	return AFUNC_OK;
}

void *  hotdm::DataGet (unsigned int vid)
{
	static int retval = 0;
	
	switch ( vid ) {
		case ID_RESO:  
		  return &resolution;
		  break;
		case ID_GLSC:
			// Enveloped parameters return their ID
			return globalScale->ID();
			break;
		case ID_WAVH:     
			return waveHeight->ID();
			break;
		case ID_SWAV:    
			return shortestWave->ID();
			break;
		case ID_DCHP:
			// LW is expecting an &int NOT a boolean
			// So we need to cast it
			retval = (int) doChop;
			return &retval;
			break;
		case ID_CHOP:
			return choppiness->ID();
			break;
		case ID_WSPD:    
			return windSpeed->ID();
			break;
		case ID_WDIR:    
			return windDirection->ID();
			break;
		case ID_WALG:    
			return windAlign->ID();
			break;
		case ID_DREF:    
			return dampReflections->ID();
			break;
		case ID_ODEP:
			return oceanDepth->ID();
			break;
		case ID_SEED:
			return &seed;
			break;
		case ID_INTP:
			// LW is expecting an &int NOT a boolean
			// So we need to cast it
			retval = (int) interpolation;
			return &retval;
			break;
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		/*case ID_NORM:
			retval = (int) normals;
			return &retval;
			break;
		case ID_JACB:
			retval = (int) jacobian;
			return &retval;
			break;*/
		
		default:
			return NULL;
			break;
	}
}

LWXPRefreshCode  hotdm::DataSet (unsigned int vid, void *value)
{

	int *i = ( int * ) value;
	bool *b = (bool *) value;

	switch ( vid ) {
		case ID_RESO:  
			resolution = *i;
			return LWXPRC_DFLT;
		case ID_DCHP:
			doChop = *b;
			return LWXPRC_DFLT;
		case ID_SEED:
			seed = *i;
			return LWXPRC_DFLT;
		case ID_INTP:
			interpolation = *b;
			return LWXPRC_DFLT;
		
		/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
		/*case ID_NORM:
			normals = *b;
			return LWXPRC_DFLT;
		case ID_JACB:
			jacobian = *b;
			return LWXPRC_DFLT;*/
		
		default:
			return LWXPRC_NONE;
	}
}


/*
======================================================================
	Defines for load / save parameters
====================================================================== */

#define IO_RESO LWID_('R','E','S','O')	//!< resolution
#define IO_GLSC LWID_('G','L','S','C')	//!< globalScale
#define IO_WAVH LWID_('W','A','V','H')	//!< waveHeigth
#define IO_SWAV LWID_('S','W','A','V')	//!< shortestWave
#define IO_DCHP LWID_('D','C','H','P')	//!< doChop
#define IO_CHOP LWID_('C','H','O','P')	//!< choppiness
#define IO_WSPD LWID_('W','S','P','D')	//!< windSpeed
#define IO_WDIR LWID_('W','D','I','R')	//!< windDirection
#define IO_WALG LWID_('W','A','L','G')	//!< windAlign
#define IO_DREF LWID_('D','R','E','F')	//!< dampReflections
#define IO_ODEP LWID_('O','D','E','P')	//!< oceanDepth
#define IO_SEED LWID_('S','E','E','D')	//!< seed
#define IO_INTP LWID_('I','N','T','P')	//!< interpolation
#define IO_STOP LWID_('S','T','O','P')  //!< End plugin

/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
//#define IO_NORM LWID_('N','O','R','M')	//!< normals
//#define IO_JACB LWID_('J','A','C','B')	//!< jacobian


static LWBlockIdent idroot[] = 
{
	IO_RESO, "resolution",
	IO_GLSC, "globalScale",
	IO_WAVH, "waveHeigth",
	IO_SWAV, "shortestWave",
	IO_DCHP, "doChop",
	IO_CHOP, "choppiness",
	IO_WSPD, "windSpeed",
	IO_WDIR, "windDirection",
	IO_WALG, "windAlign",
	IO_DREF, "dampReflections",
	IO_ODEP, "oceanDepth",
	IO_SEED, "seed",
	IO_INTP, "interpolation", // 12 elements
	IO_STOP, "EndPlugin", // 13 elements
	/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
	//IO_NORM, "normals", 
	//IO_JACB, "jacobian",	// 14 elements
	0
};

/* ======================================================================
Load()
====================================================================== */
LWError hotdm::Load( const lwpp::LoadState &ls )
{
	LWError err = NULL;

	//! Settings to read:
	while(LWID id = ls.Find(idroot)) 
	{
		switch( id ) 
		{
			case IO_RESO: 
				resolution = ls.readInt();
				break;

			case IO_GLSC: 
				globalScale->Load(ls);
				break;

			case IO_WAVH: 
				waveHeight->Load(ls);
				break;

			case IO_SWAV: 
				shortestWave->Load(ls);
				break;

			case IO_DCHP: 
				doChop = ls.readBool();
				break;

			case IO_CHOP: 
				choppiness->Load(ls);
				break;

			case IO_WSPD: 
				windSpeed->Load(ls);
				break;

			case IO_WDIR: 
				windDirection->Load(ls);
				break;

			case IO_WALG: 
				windAlign->Load(ls);
				break;

			case IO_DREF: 
				dampReflections->Load(ls);
				break;

			case IO_ODEP: 
				oceanDepth->Load(ls);
				break;

			case IO_SEED: 
				seed = ls.readInt();
				break;

			case IO_INTP: 
				interpolation = ls.readBool();
				break;
			
			/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
			/*case IO_NORM: 
				normals = ls.readBool();
				break;

			case IO_JACB: 
				jacobian = ls.readBool();
				break;*/

			// This avoid a LW error, that will overwrite the values of the plugin if there
			// is more than ONE instance of the plugin applied to the object.
			case IO_STOP: return err;

			default:
			break;
		}

	ls.End();
	}

	return err;
}


/* ======================================================================
LoadSettings()
====================================================================== */
LWError hotdm::LoadSettings(const lwpp::LoadState &ls, bool is_preset)
{
LWError err = NULL;

	//! Settings to read:
	while(LWID id = ls.Find(idroot)) 
	{
		switch( id ) 
		{
			case IO_RESO: 
				resolution = ls.readInt();
				break;

			case IO_GLSC: 
				globalScale->Load(ls);
				break;

			case IO_WAVH: 
				waveHeight->Load(ls);
				break;

			case IO_SWAV: 
				shortestWave->Load(ls);
				break;

			case IO_DCHP: 
				doChop = ls.readBool();
				break;

			case IO_CHOP: 
				choppiness->Load(ls);
				break;

			case IO_WSPD: 
				windSpeed->Load(ls);
				break;

			case IO_WDIR: 
				windDirection->Load(ls);
				break;

			case IO_WALG: 
				windAlign->Load(ls);
				break;

			case IO_DREF: 
				dampReflections->Load(ls);
				break;

			case IO_ODEP: 
				oceanDepth->Load(ls);
				break;

			case IO_SEED: 
				seed = ls.readInt();
				break;

			case IO_INTP: 
				interpolation = ls.readBool();
				break;
			
			/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
			/*case IO_NORM: 
				normals = ls.readBool();
				break;

			case IO_JACB: 
				jacobian = ls.readBool();
				break;*/

			// This avoid a LW error, that will overwrite the values of the plugin if there
			// is more than ONE instance of the plugin applied to the object.
			case IO_STOP: return err;

			default:
			break;
		}

	ls.End();
	}

	return err;
}

/* ======================================================================
Save()
====================================================================== */

LWError hotdm::Save( const lwpp::SaveState &ss )
{ 

	ss.WriteBlock(idroot[0], resolution);
	ss.WriteItem(idroot[1], *globalScale);
	
	ss.WriteItem(idroot[2], *waveHeight);
	ss.WriteItem(idroot[3], *shortestWave);
	ss.WriteBlock(idroot[4], doChop);
	ss.WriteItem(idroot[5], *choppiness);

	ss.WriteItem(idroot[6], *windSpeed);
	ss.WriteItem(idroot[7], *windDirection);
	ss.WriteItem(idroot[8], *windAlign);


	ss.WriteItem(idroot[9], *dampReflections);
	ss.WriteItem(idroot[10], *oceanDepth);

	ss.WriteBlock(idroot[11], seed);

	ss.WriteBlock(idroot[12], interpolation);

	/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
	/*ss.WriteBlock(idroot[13], normals);
	ss.WriteBlock(idroot[14], jacobian);*/

	return NULL;
}


/* ======================================================================
SaveSettings()
====================================================================== */
LWError hotdm::SaveSettings(const lwpp::SaveState &ss, bool is_preset)
{
	ss.WriteBlock(idroot[0], resolution);
	ss.WriteItem(idroot[1], *globalScale);
	
	ss.WriteItem(idroot[2], *waveHeight);
	ss.WriteItem(idroot[3], *shortestWave);
	ss.WriteBlock(idroot[4], doChop);
	ss.WriteItem(idroot[5], *choppiness);

	ss.WriteItem(idroot[6], *windSpeed);
	ss.WriteItem(idroot[7], *windDirection);
	ss.WriteItem(idroot[8], *windAlign);


	ss.WriteItem(idroot[9], *dampReflections);
	ss.WriteItem(idroot[10], *oceanDepth);

	ss.WriteBlock(idroot[11], seed);

	ss.WriteBlock(idroot[12], interpolation);

	/* This DOESNT work for displacement plugins, But left the code to see how other controls can be done */
	/*ss.WriteBlock(idroot[13], normals);
	ss.WriteBlock(idroot[14], jacobian);*/

	return NULL;
}



extern "C"
ServerTagInfo hotdm_Tags[] =
{
	{ "HOT Ocean Toolkit", SRVTAG_USERNAME },
	{ "HOT Ocean Toolkit", SRVTAG_DESCRIPTION },
	{ NULL }
};

lwpp::XPanelDisplacementAdaptor<hotdm> HOTDM_Instance("HOT_Ocean_Toolkit", hotdm_Tags);