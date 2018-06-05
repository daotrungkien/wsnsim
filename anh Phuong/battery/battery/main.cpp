#include <iostream>
#include <math.h>
#include <string>
#include <fstream>
#include "Battery.h"
#include "Lithium.h"
#include "Nikel_Cadmium.h"

using namespace std;

void main()
{
	Lithium pin1;
	//Nikel_Cadmium pin2;
	cout << "thong so pin1:" << endl;
	pin1.nhapthongsodau();
	pin1.phantrampin();
	//pin2.nhapthongsodau();
	//pin2.phantrampin();
	
	// pin1.nhapthongsodau(5,1,150,1200)
	//cout << pin1.phantrampin() << endl;
	//cout << "thoi gian con pin1: " << pin1.thoigianconlai() << endl;

	//cout << "thong so pin2:" << endl;
	//pin2.nhapthongsodau(3, 1, 160, 800);
	//cout<<pin2.phantrampin() << endl;
	//battery *tp = new battery();
	//tp->thoigianconlai();
	//cout << "thoi gian con pin2: " << pin2.thoigianconlai() << endl;

	system("pause");

}
