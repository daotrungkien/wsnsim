#pragma once
#include "Battery.h"

class Lithium :	public Battery
{
protected:
	double Rin, Inom;
public:
	Lithium();
	virtual ~Lithium();
	virtual double getQmax();
	virtual double getEcharge();
	virtual double getEdischarge();
};

