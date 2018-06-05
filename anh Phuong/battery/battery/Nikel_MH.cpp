#include "Nikel_MH.h"
#include <math.h>




Nikel_MH::Nikel_MH()
{
	double Rin = (U_batt / Q_rated) / 100;
	double Inom = 0.2*Q_rated;
}
double Nikel_MH::getEdischarge()
{// Theo reference cua pin trong Matlab
 //double U_batt;
 //double e = exp(-26.5487*Qdt);
U_batt = 1.164*U_battnom - 0.01*I_load - (0.0076*getQmax() * 2 * I_load) / (getQmax() - Qdt) + 0.26422*exp(-26.5487*Qdt);
	// coi i* bang I_load
	return U_batt;
}

Nikel_MH::~Nikel_MH()
{
}

double Nikel_MH::getQmax()
{
	return 1.07692 * Q_rated;
}