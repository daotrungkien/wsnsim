#include "Lithium.h"
#include <math.h>


Lithium::Lithium()
{

	double Rin = (U_battnom / Q_rated) / 100;
	double Inom = 0.4348*Q_rated;
}

double Lithium::getQmax()
{
	return Q_rated;
}
double Lithium::getEdischarge()
{// Theo reference cua pin trong Matlab
	//double U_batt;
	//double e = exp(-26.5487*Qdt);
	//U_batt = 1.164*U_battnom - 0.01*I_load - (0.0076*getQmax()*2*I_load) / (getQmax()-Qdt) + 0.26422*exp(-26.5487*Qdt);
	//U_batt = 1.164*U_battnom - Rin*I_load - (((0.0076*getQmax()) / (getQmax() - Qdt))*(I_load + Qdt)) + 3.5653*(exp(-26.5487*Qdt));
	// coi i* bang I_load
	return U_batt;
}

double Lithium::getEcharge()
{
	return Q_rated;
}


Lithium::~Lithium()
{
}
