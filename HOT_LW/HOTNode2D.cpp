#include "HOTNode2D.h"

HOTNode2D::HOTNode2D( void *priv, void *context, LWError *err) :
					  HOTNode(priv, context, err)
{
					  
}

// Destructor
HOTNode2D::~HOTNode2D() { ; }


/* ======================================================================
Evaluate()
====================================================================== */
void  HOTNode2D::Evaluate(LWNodalAccess *na, NodeOutputID outID, NodeValue value)
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
	
	// For some strange reason there is no need to lock on Image or Pixel 
	// Filters when this node is used inside on Dennis Image or Pixel Filter nodes.
	// But if this node is used on the Nodal Surface editor it crashes unles the locks
	// are in, because multiple threads want to delete the ocean several times.
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

	// Do CatmullRom interpolation, or linear
	if (doInterpolation)
	{
		_ocean_context->eval_ij( na->sx, na->sy);
	}
	
	lwpp::Point3d tmpPoint(0.0, 0.0, 0.0);

	// Only do the chopinnes if is active 
	// and normals inactive (because we do not delete the Do normals connection if Do Chop is activatechoppines).
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
	// and chopinnes inactive (because we do not delete the connection if you change choppines.
	if (doNormals)
	{
		// Careful with things like this: (outNormal->isID(outNormal))
		// That will not work on a million years! :)
		if (outNormal->isID(outID))
		{
			// Ocean.h store the normals calculated on _ocean_context->eval2_xz
			// into the _ocean_context->normal
			
				// A zero vector. 
				lwpp::Vector3d normal(0.0, 0.0, 0.0);
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
			if(outFoam->isID(outID) && doChop)
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


ServerTagInfo HOTNode2D_tags[] =
{
	{ "HOT Ocean Toolkit 2D", SRVTAG_USERNAME },
	{ "HOT Ocean Toolkit", SRVTAG_NODEGROUP},
	{ "HOT Ocean Toolkit Node 2D", SRVTAG_DESCRIPTION },
	{ NULL }
};

// Register the plugin indicating the minimum version of the 
lwpp::XPanelNodeAdaptor<HOTNode2D, 2, 2> HOTNode2D_Instance("HOT Ocean Toolkit Node 2D", HOTNode2D_tags);