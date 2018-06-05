#include "Nikel_Cadmium.h"



Nikel_Cadmium::Nikel_Cadmium()
{
	double Rin = (U_batt / Q_rated) / 100;// noi tro cua pin
	double Inom = 0.2*Q_rated;

}


Nikel_Cadmium::~Nikel_Cadmium()
{
}

double Nikel_Cadmium::getQmax() 
{	
	return 1.1364 * Q_rated;
}
