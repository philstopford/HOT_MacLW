#include "HOTNode.h"


HOTNode::HOTNode( void *priv, void *context, LWError *err) :
				  lwpp::XPanelNodeHandler(priv, context, err),
				  // LWNODE_HCLASS also defines the directory, if you want this shared across different types of plugins, use a common string here
				  Preset(LWNODE_HCLASS, "HOT Ocean Toolkit"),
				  nCores(0),
				  resolution(8),
				  doChop(true),
				  seed(0),
				  doInterpolation(true),
				  doNormals(false),
				  doJacobian(false),
				  outDisplacement(0),
				  outNormal(0),
				  outFoam(0),
				  outSpray(0),
				  outJMinus(0),
				  outJPlus(0),
				  EignMinus(0),
				  EignPlus(0),
				  thGroup(1)
{
	if ( !lwpp::LightWave::isAtLeast(9, 6) )
	{
		*err = "This plugin needs a newer version of LightWave 3D (9.6 as a minimum)";
		return;
	}

	// Get the number of cores of the computer
	nCores =  lwpp::LightWave::GetNumCores();
	
	// Do this in Layout only to avoid crash modeler
	if( lwpp::LightWave::isLayout())
	{
		// Store the LWItemID of the object which the plugin is applied
		itemID.SetID( (LWItemID) context);
	}

	// Initialize the parameters of the plugin
	// This are variant type of LWVP_DIST because is measured in meters
	globalScale = new lwpp::VParm(LWVP_DIST, "globalScale", "HOT Ocean Toolkit", chanGrp(), 50.0);	
	waveHeight = new lwpp::VParm(LWVP_DIST, "waveHeight", "HOT Ocean Toolkit", chanGrp(), 1.6);
	shortestWave = new lwpp::VParm(LWVP_DIST, "shortestWave", "HOT Ocean Toolkit", chanGrp(), 0.02);
	choppiness = new lwpp::VParm(LWVP_DIST, "choppiness", "HOT Ocean Toolkit", chanGrp(), 1.78);
	windSpeed = new lwpp::VParm(LWVP_DIST, "windSpeed", "HOT Ocean Toolkit", chanGrp(), 30.0);
	
	// This is a variant type of LWVP_ANGLE because is measured in degrees
	windDirection = new lwpp::VParm(LWVP_ANGLE, "windDirection", "HOT Ocean Toolkit", chanGrp(), 0.0);
	
	// This is a variant type of LWVP_FLOAT because it lacks of units
	windAlign = new lwpp::VParm(LWVP_FLOAT, "windAlign", "HOT Ocean Toolkit", chanGrp(), 2.0);
	dampReflections = new lwpp::VParm(LWVP_FLOAT, "dampReflections", "HOT Ocean Toolkit", chanGrp(), 0.5);

	// This is a variant type of LWVP_DIST because is measured in meters
	oceanDepth = new lwpp::VParm(LWVP_DIST, "oceanDepth", "HOT Ocean Toolkit", chanGrp(), 200.0);

	// Node Inputs 
	inResolution =	addIntegerInput("Resolution");
	inGlobalScale =	addScalarInput("Global Scale");
	inWaveHeight =	addScalarInput("Wave Height");
	inShortestWave =	addScalarInput("Shortest Wave");
	inChoppiness =	addScalarInput("Choppiness");
	inWindSpeed =	addScalarInput("Wind Speed");
	inWindDirection =	addScalarInput("Wind Direction");
	inWindAlign =	addScalarInput("Wind Align");
	inDampReflections =	addScalarInput("Damp Reflections");
	inOceanDepth =	addScalarInput("Ocean Depth");
	inSeed =	addIntegerInput("Seed");

	// Node Outputs
	outDisplacement = addVectorOutput("Displacement");
	// This output is controlled by the state of the checkBox Do Normals
	// Only add the outNormal output if doNormals is active
	if (doNormals)
	{
		outNormal = addVectorOutput("Normal");
	}

	if (doJacobian)
	{
		if (doChop && !outFoam)
		{
			// The Foam output only has sense if chopiness is active
			outFoam = addVectorOutput("Foam");
		}
		
		outSpray = addVectorOutput("Spray");
		outJMinus = addScalarOutput("JMinus");
		outJPlus = addScalarOutput("JPlus");
		EignMinus = addVectorOutput("EigenMinus");
		EignPlus= addVectorOutput("EigenPlus");
	}

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

HOTNode::~HOTNode()
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

	// Inputs
	if (inResolution)		delete inResolution;
	if (inGlobalScale)		delete inGlobalScale;
	if (inWaveHeight)		delete inWaveHeight;
	if (inShortestWave)		delete inShortestWave;
	if (inChoppiness)		delete inChoppiness;
	if (inWindSpeed)		delete inWindSpeed;
	if (inWindDirection)	delete inWindDirection;
	if (inWindAlign)		delete inWindAlign;
	if (inDampReflections)	delete inDampReflections;
	if (inOceanDepth)		delete inOceanDepth;
	if (inSeed)				delete inSeed;
					
	// Outputs
	if (outDisplacement)	delete outDisplacement;
	if (outNormal)			delete outNormal;
	if (outFoam)			delete outFoam;
	if (outSpray)			delete outSpray;
	if (outJMinus)			delete outJMinus;
	if (outJPlus)			delete outJPlus;
	if (EignMinus)			delete EignMinus;
	if (EignPlus)			delete EignPlus;
	
}

// Asigment operator 
HOTNode &HOTNode::operator=(const HOTNode &from)
{
	// [15:58:17] Michael Wolf: When you save a preset, LW first copies the plugin, and then saves it!
	// [15:58:34] Michael Wolf: And you need to make sure in the copy and the new copy also has the matching outputs activated!
	// [15:58:53] Michael Wolf: Othwerise - no output, no connection.
	if (&from != this)
	{
		resolution = from.resolution;
		*globalScale = *from.globalScale;
		*waveHeight = *from.waveHeight;
		*shortestWave = *from.shortestWave;
		doChop = from.doChop;
		*choppiness = *from.choppiness;
		*windSpeed = *from.windSpeed;
		*windDirection = *from.windDirection;
		*windAlign = *from.windAlign;
		*dampReflections = *from.dampReflections;
		*oceanDepth = *from.oceanDepth;
		seed = from.seed;
		doInterpolation = from.doInterpolation;
		doNormals = from.doNormals;

		if (doNormals)
		{
			outNormal = addVectorOutput("Normal");
		}

		doJacobian = from.doJacobian;

		if (doJacobian)
		{
			if (doChop && !outFoam)
			{
				// The Foam output only has sense if chopiness is active
				outFoam = addVectorOutput("Foam");
			}

			outSpray = addVectorOutput("Spray");
			outJMinus = addVectorOutput("JMinus");
			outJPlus = addVectorOutput("JPlus");
			EignMinus = addVectorOutput("EigenMinus");
			EignPlus = addVectorOutput("EigenPlus");
		}


	}
	return *this;
	
}

/* ======================================================================
Evaluate()
====================================================================== */
void  HOTNode::Evaluate(LWNodalAccess *na, NodeOutputID outID, NodeValue value)
{

	int res = resolution;
	inResolution->evaluate(na, &res);
	// Avoid a crash if the users connects for example
	// a int node with a negative value.
	res = (res < 1) ? 1 : res;
	int gridres  = 1 << int(res);

	double stepsize = globalScale->GetValuef();
	inGlobalScale->evaluate(na, &stepsize);

	stepsize /= (float) gridres;

	double wHeight = waveHeight->GetValuef();
	inWaveHeight->evaluate(na, &wHeight);

	double sWave = shortestWave->GetValuef();
	inShortestWave->evaluate(na, &sWave);

	double chpnss = choppiness->GetValuef();
	inChoppiness->evaluate(na, &chpnss);
	
	double wSpeed = windSpeed->GetValuef();
	inWindSpeed->evaluate(na, &wSpeed);

	// Ocean.h expect the win direction in radians
	double wDir = windDirection->GetValuef();
	inWindDirection->evaluate(na, &wDir);
	
	double wAlign = windAlign->GetValuef();
	inWindAlign->evaluate(na, &wAlign);

	double tmpDampRef = dampReflections->GetValuef();
	inDampReflections->evaluate(na, &tmpDampRef);
	float dRef = 1.0f - tmpDampRef;
	
	double oDepth = oceanDepth->GetValuef();
	inOceanDepth->evaluate(na, &oDepth);
	
	int tmpSeed = seed;
	inSeed->evaluate(na, &tmpSeed);

	thGroup.lockMutex(lwpp::MUTEX0);

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
									tmpSeed);	// seed in Ocean.h
        
		_ocean_scale   = _ocean->get_height_normalize_factor();

		_ocean_context = _ocean->new_context(true,doChop,doNormals,doJacobian);

		// sum up the waves at this timestep
		_ocean->update( (float) newTime,
								*_ocean_context,
								true,
								doChop,
								doNormals,
								doJacobian,
								_ocean_scale * (float) wHeight,
								chpnss);

        _ocean_needs_rebuild = false;

    }

	// Get the position of the point on world coordinates
	lwpp::Point3d point(na->wPos);

	// Do CatmullRom interpolation, or linear
	if (doInterpolation)
	{
		_ocean_context->eval2_xz( (float) point.x, (float) point.z);
	} else 
	{
		_ocean_context->eval_xz( (float) point.x, (float) point.z);
	}
	
	lwpp::Point3d tmpPoint(point);

	// Only do the chopinnes if is active 
	// and normals inactive (because we do not delete the Do normals connection if Do Chop is active).
	if (doChop) 
    {
		// On the nodal version we DO NOT add the vector
		// The node adds it for us
		tmpPoint.x = _ocean_context->disp[0];
		tmpPoint.y = _ocean_context->disp[1];
		tmpPoint.z = _ocean_context->disp[2];
    }
    else
    {
		// On the nodal version we DO NOT add the vector
		// The node adds it for us. 
		// x and y MUST be 0, or it will be like addd the point to itself,
		// doubling the size of the ocean. Try it out commenting the .x .y lines.
		tmpPoint.x = 0;
		tmpPoint.y = _ocean_context->disp[1];
		tmpPoint.z = 0;
    }

	if (outDisplacement->isID(outID))
	{
		setValue(value, tmpPoint.asLWVector());
	}

	// Only check the outNormal output if doNormals is active
	// and chopinnes inactive (because we do not delete the connection if you change choppines).
	if (doNormals)
	{
		// Careful with things like this: (outNormal->isID(outNormal))
		// That will not work on a million years! :)
		if (outNormal->isID(outID))
		{
			// Ocean.h store the normals calculated on _ocean_context->eval2_xz
			// into the _ocean_context->normal
			
				// The interpolated normal in world coordinates. 
				lwpp::Vector3d normal(na->wNorm0);
				// Geometric normal in world coordinates. This is the raw polygonal normal at the spot, unperturbed by smoothing or bump mapping. 
				// For a different and slightly sharpen result use this one
				// lwpp::Vector3d normal(na->gNorm);

				lwpp::Vector3d onormal( _ocean_context->normal[0],
										_ocean_context->normal[1],
										_ocean_context->normal[2]);
				onormal.Normalize();

				normal += onormal;

				// If we DO NOT add onormal to LW normal, 
				// the result is more sharpened, but looks that ignores the 
				// normals altered by the displacement. Check with someone! ;)
				setValue(value, normal);
		}
	}

	if (doJacobian)
	{
		
		if (doChop && outFoam)
		{
			if(outFoam->isID(outID))
			{
				lwpp::Vector3d vfoam(0.0, 0.0,0.0);
				// lwpp::Vector3d vjacobian( _ocean_context->Eminus[0], 0.0, _ocean_context->Eminus[1]);
				
				double jm = (double) _ocean_context->Jminus; 
				
				if (jm < 0.0)
				{
					vfoam.x = -jm;  
					vfoam.y = -jm;  
					vfoam.z = -jm;  
				}

				// Drew recommended do this on his email, but not sure it works better 
				// than simply left the output unmodified. In fact
				// looks like the unmodified output works better when it's connected to a gradient. 
				// Or connect it to a Vector2 Scalar node and use Maximum , Minimun, X, Y, or Z as options.
				// vfoam *=  _ocean_scale;

				setValue(value, vfoam.asLWVector());
			
			}
		}

		if(outSpray->isID(outID))
		{
			lwpp::Vector3d vspray(0.0, 0.0,0.0);
			// lwpp::Vector3d vjacobian( _ocean_context->Eminus[0], 0.0, _ocean_context->Eminus[1]);
			
			double jm = (double) _ocean_context->Jminus; 

			// Posible calculation for spray particles
			double jt = 1.0;

			if (jm < jt)
			{
				vspray.x = -jm;  
				vspray.y = -jm;  
				vspray.z = -jm;  
			}

			double jt_jm = jt - jm; 

			// The interpolated normal in world coordinates. 
			lwpp::Vector3d normal(na->wNorm0);

			lwpp::Vector3d veminus( _ocean_context->Eminus[0], _ocean_context->Eminus[1], _ocean_context->Eminus[2]);

			veminus *= jt_jm;

			vspray = veminus + normal / sqrt ( 1.0 + (jt_jm * jt_jm));

			setValue(value, vspray.asLWVector());
		
		}

		// Careful with things like this: (outJMinus->isID(outJMinus))
		// That will not work on a million years! :)
		if (outJMinus->isID(outID))
		{
				double jm = (double) _ocean_context->Jminus; 
				
				if (jm < 0.0)
				{
					jm = -jm;
				}
				// Drew recommended do this on his email, but not sure it works better 
				// than simply left the output unmodified. In fact
				// looks like the unmodified output works better when it's connected to a gradient. 
				// Or connect it to a Vector2 Scalar node and use Maximum , Minimun, X, Y, or Z as options.
				// jm *=  _ocean_scale;
				setValue(value, jm);

		}
	
		if (outJPlus->isID(outID))
		{
				
				double jp = (double) _ocean_context->Jplus;
				setValue(value, jp);

		}

		if (EignMinus->isID(outID))
		{
				// lwpp::Vector3d veminus( _ocean_context->Eminus[0], 0.0, _ocean_context->Eminus[1]);
				lwpp::Vector3d veminus( _ocean_context->Eminus[0], _ocean_context->Eminus[1], _ocean_context->Eminus[2]);
				veminus.Normalize();
				
				setValue(value, veminus.asLWVector());

		}


		if (EignPlus->isID(outID))
		{
				lwpp::Vector3d veplus( _ocean_context->Eplus[0], _ocean_context->Eplus[1], _ocean_context->Eplus[2]);
				veplus.Normalize();
				
				setValue(value, veplus.asLWVector());
		}
	} //end if (doJacobian)

	thGroup.unlockMutex(lwpp::MUTEX0);
	
}

/* ======================================================================
NewTime()
====================================================================== */
LWError HOTNode::NewTime( LWFrame frame, LWTime time ) 
{ 
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
	
	// Store the time to use it in evaluate
	newTime = time;

	// LAST CORRECTION UNCOMENTED THIS
	// thGroup.lockMutex(lwpp::MUTEX0);

	// We indicate that the ocean needs rebuild on the evaluate
	_ocean_needs_rebuild = true;

	// LAST CORRECTION UNCOMENTED THIS
	// thGroup.unlockMutex(lwpp::MUTEX0);

	UNUSED(frame);
	return NULL;
}


/* ======================================================================
Interface()
====================================================================== */
int HOTNode::Interface( long version, LWInterface *local, void *serverData )
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
		{ID_NORM, "Do Normals",					"iBoolean"},
		{ID_JACB, "Jacobian",					"iBoolean"},
		{ID_PRES, "Presets",					"vPopCmd"},
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
		{ID_NORM, "Do Normals",					"integer"},
		{ID_JACB, "Jacobian",					"integer"},
		{ID_PRES, "Presets",					"integer"},
		{ 0 }
	};
	
	static LWXPanelHint hints[] = {
		XpLABEL( 0, "HOT Ocean Toolkit" ),
		// Enable Use Choppiness and  Chopinnes controls
		XpENABLE_(ID_DCHP),XpH(ID_CHOP),XpEND,
		XpENABLE_MAP_(ID_DCHP, nnrm_map), XpH(ID_NORM),XpEND,
		
		XpDIVADD( ID_GLSC ), //Separator after Global Scale
		XpDIVADD( ID_SWAV ),
		XpDIVADD( ID_CHOP ),
		XpDIVADD( ID_WALG ),
		XpDIVADD( ID_ODEP ),
		XpDIVADD( ID_JACB ),

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

void *  HOTNode::DataGet (unsigned int vid)
{
	static int retval = 0;

	/*  Return NULL to disable the control,
        if there is something connected 
		to the corresponding input.			*/
	
	switch ( vid ) {
		case ID_RESO:
			// If the input doesn't have a conection, return the usual value
			if (!inResolution->isConnected()) return &resolution;
			return 0;
			break;
		case ID_GLSC:
			// If the input doesn't have a conection, return the envelope
			if (!inGlobalScale->isConnected()) return globalScale->ID();
			return 0;
			break;
		case ID_WAVH:  
			if (!inWaveHeight->isConnected()) return waveHeight->ID();
			return 0;
			break;
		case ID_SWAV:
			if (!inShortestWave->isConnected()) return shortestWave->ID();
			return 0;
			break;
		case ID_DCHP:
			// LW is expecting an &int NOT a boolean
			// So we need to cast it
			retval = (int) doChop;
			return &retval;
			break;
		case ID_CHOP:
			if (!inChoppiness->isConnected()) return choppiness->ID();
			return 0;
			break;
		case ID_WSPD:
			if (!inWindSpeed->isConnected()) return windSpeed->ID();
			return 0;
			break;
		case ID_WDIR:
			if (!inWindDirection->isConnected()) return windDirection->ID();
			return 0;
			break;
		case ID_WALG:
			if (!inWindAlign->isConnected()) return windAlign->ID();
			return 0;
			break;
		case ID_DREF: 
			if (!inDampReflections->isConnected()) return dampReflections->ID();
			return 0;
			break;
		case ID_ODEP:
			if (!inOceanDepth->isConnected()) return oceanDepth->ID();
			return 0;
			break;
		case ID_SEED:
			if (!inSeed->isConnected()) return &seed;
			return 0;
			break;
		case ID_INTP:
			// LW is expecting an &int NOT a boolean
			// So we need to cast it
			retval = (int) doInterpolation;
			return &retval;
			break;
		case ID_NORM:
			retval = (int) doNormals;
			if (!doNormals)
			{
				if (outNormal)
				{
				  delete outNormal;
				  outNormal = NULL;
				}
			}
			return &retval;
			break;
		case ID_JACB:
			retval = (int) doJacobian;
			if (!doJacobian)
			{
				// delete the outputs
				if(!doChop && outFoam)
				{
					delete outFoam;
					outFoam = NULL;
				}
				if(outSpray)
				{
					delete outSpray;
					outSpray = NULL;
				}
				if (outJMinus)
				{
				  delete outJMinus;
				  outJMinus = NULL;
				}
				if (outJPlus)
				{
				  delete outJPlus;
				  outJPlus = NULL;
				}
				if (EignMinus)
				{
				  delete EignMinus;
				  EignMinus = NULL;
				}
				if (EignPlus)
				{
				  delete EignPlus;
				  EignPlus = NULL;
				}
			}
			return &retval;
			break;
		default:
			return NULL;
			break;
	}
}

LWXPRefreshCode  HOTNode::DataSet (unsigned int vid, void *value)
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
			doInterpolation = *b;
			return LWXPRC_DFLT;
		case ID_NORM:
			doNormals = *b;
			if (doNormals)
				outNormal = addVectorOutput("Normal");
			return LWXPRC_DFLT;
		case ID_JACB:
			doJacobian = *b;
			if (doJacobian)
			{
				if (doChop && !outFoam)
				{
					// The Foam output only has sense if chopiness is active
					outFoam = addVectorOutput("Foam");
				}
				outSpray = addVectorOutput("Spray");
				outJMinus = addScalarOutput("JMinus");
				outJPlus = addScalarOutput("JPlus");
				EignMinus = addVectorOutput("EigenMinus");
				EignPlus= addVectorOutput("EigenPlus");
			}
			return LWXPRC_DFLT;
		default:
			return LWXPRC_NONE;
	}
}

void HOTNode::ChangeNotify (LWXPanelID panel, unsigned int cid, unsigned int vid, int event_type)
{
     if (event_type == LWXPEVENT_TRACK)
     {
       UpdateNodePreview();
     }
     else if (event_type == LWXPEVENT_VALUE)
     {
       Update();
     }
}



/*
/* ======================================================================
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
#define IO_NORM LWID_('N','O','R','M')	//!< normals
#define IO_JACB LWID_('J','A','C','B')	//!< jacobian
// #define IO_STOP LWID_('S','T','O','P')  //!< End plugin


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
	IO_INTP, "doInterpolation",
	IO_NORM, "doNormals", 
	IO_JACB, "doJacobian",	// 14 elements
	0
};

/* ======================================================================
Load()
====================================================================== */
LWError HOTNode::Load( const lwpp::LoadState &ls )
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
				doInterpolation = ls.readBool();
				break;
			
			case IO_NORM: 
				doNormals = ls.readBool();
				// Only add the outNormal output if doNormals is active
				// on Load
				if (doNormals)
				{
					outNormal = addVectorOutput("Normal");
				}
				break;

			case IO_JACB: 
				doJacobian = ls.readBool();
				// Only add this outputs if doJacobian is active
				if (doJacobian)
				{
					if (doChop && !outFoam)
					{
						// The Foam output only has sense if chopiness is active
						outFoam = addVectorOutput("Foam");
					}
					outSpray = addVectorOutput("Spray");
					outJMinus = addVectorOutput("JMinus");
					outJPlus = addVectorOutput("JPlus");
					EignMinus = addVectorOutput("EigenMinus");
					EignPlus= addVectorOutput("EigenPlus");
				}
				// This avoids a problem loading presets, cpoying an pasting nodes and losing the conections.
				ls.End();
				return err;
				break;

			default:
			break;
		}
		// End of the reading 
		ls.End();

	} // end while

	return err;
}


/* ======================================================================
Save()
====================================================================== */

LWError HOTNode::Save( const lwpp::SaveState &ss )
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

	ss.WriteBlock(idroot[12], doInterpolation);
	ss.WriteBlock(idroot[13], doNormals);
	ss.WriteBlock(idroot[14], doJacobian);

	return NULL;
}

/* ======================================================================
LoadSettings()
====================================================================== */
LWError HOTNode::LoadSettings(const lwpp::LoadState &ls, bool is_preset)
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
				doInterpolation = ls.readBool();
				break;
			
			case IO_NORM: 
				doNormals = ls.readBool();
				// Only add the outNormal output if doNormals is active
				// on Load
				if (doNormals)
				{
					outNormal = addVectorOutput("Normal");
				}
				break;

			case IO_JACB: 
				doJacobian = ls.readBool();
				// Only add this outputs if doJacobian is active
				if (doJacobian)
				{
					if (doChop && !outFoam)
					{
						// The Foam output only has sense if chopiness is active
						outFoam = addVectorOutput("Foam");
					}
					outSpray = addVectorOutput("Spray");
					outJMinus = addVectorOutput("JMinus");
					outJPlus = addVectorOutput("JPlus");
					EignMinus = addVectorOutput("EigenMinus");
					EignPlus= addVectorOutput("EigenPlus");
				}
				// This avoids a problem loading presets, cpoying an pasting nodes and losing the conections.
				ls.End();
				return err;
				break;

			default:
			break;
		}
		// End of the reading 
		ls.End();

	} // end while

	return err;
}
		

/* ======================================================================
SaveSettings()
====================================================================== */
LWError HOTNode::SaveSettings(const lwpp::SaveState &ss, bool is_preset)
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

	ss.WriteBlock(idroot[12], doInterpolation);
	ss.WriteBlock(idroot[13], doNormals);
	ss.WriteBlock(idroot[14], doJacobian);

	return NULL;
}



ServerTagInfo HOTNode_tags[] =
{
	{ "HOT Ocean Toolkit", SRVTAG_USERNAME },
	{ "HOT Ocean Toolkit", SRVTAG_NODEGROUP},
	{ "HOT Ocean Toolkit Node", SRVTAG_DESCRIPTION },
	{ NULL }
};

// Register the plugin indicating the minimum version of the 
lwpp::XPanelNodeAdaptor<HOTNode, 2, 2> HOTNode_Instance("HOT Ocean Toolkit Node", HOTNode_tags);
