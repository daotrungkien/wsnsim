#pragma once
#include "Battery.h"
class Nikel_MH : public Battery
{
public:
	Nikel_MH();
	virtual ~Nikel_MH();
	virtual double getQmax();
	virtual double getEdischarge();
};

