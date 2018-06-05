#pragma once
#include "Battery.h"
class Nikel_Cadmium : public Battery
{
public:
	Nikel_Cadmium();
	virtual ~Nikel_Cadmium();
	virtual double getQmax();
};

