#ifndef HOTNODE2D_H
#define HOTNODE2D_H

#include "HOTNode.h"

class HOTNode2D : public HOTNode
{

public:

	// Constructor & Destructor
	HOTNode2D (void *priv, void *context, LWError *err);
	//HOTNode2D ();
	~HOTNode2D ();


	//! Evaluate
	virtual void  Evaluate (LWNodalAccess *na, NodeOutputID outID, NodeValue value);

};


#endif