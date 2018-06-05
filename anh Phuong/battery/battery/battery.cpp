#include "Battery.h"
#include <iostream>
#include <fstream>


using namespace std;

void Battery::nhapthongsodau()//nhapthongsodau(int t1, double dienap1, double dongxa1, double dungluongmax1)
{
	cout << "Enter time rang to simulink (s): ";
	cin >> Tm;
	cout << endl; // gia dinh cho vong for
	cout << "Enter battery U_battnom (V): ";
	cin >> U_battnom;
	cout << endl;
	cout << " Enter load value R(Ohm): ";
	cin >> Rload;
	cout << endl;
	cout << " Enter rate capacity of battery (Ah): ";
	cin >> Q_rated;// dung luong danh dinh cua pin 
	cout << endl;

	cout << "Initial battery's parameters :" << U_battnom << "|| " << (I_load = U_battnom / Rload )<< "||" << Q_rated << endl;
};


void Battery::phantrampin()
{
	double soc;// mang chua dung luong hien thoi
	double t = 0; //thoi gian tuc thoi
	const double dt = 0.1;
	//double Qdt = 0;// tich phan trong khoang dt 
	double Qt;// dung luong hien tai
	//int i;
	int hsnapxa;// qua trinh nap xa xen ke nhau
	double Rin = (U_battnom / Q_rated) / 100;
	ofstream file("ketqua.txt");
	//I_load = U_battnom / Rload;
	//------ ghi ket qua vao file ------
	for (t = 0; t < Tm; t += dt)  // lay gia tri t tu ham nhapthongsodau ???
	{
		int hs;

		hs = (int)(t/10000);
		hsnapxa = ((hs % 2 == 0) ? 1 : -1);// hsnapxa =1 la qt nap; -1 la xa

		//qrated += dt*dongxa*hsnapxa;// tai sao hsnapxa ma de tinh trong bieu thuc soc lai khong duoc?????????????
		//Qdt += dt*I_load;// thu qua trinh xa
		
		Qdt +=hsnapxa*dt*I_load/3600;
		Qt = getQmax() - Qdt;
		soc = 1. - (Qdt /getQmax());//gia dinh tinh SOC cua pin
		I_batt = hsnapxa*I_load;
		//I_batt = I_load;
		//getEdischarge();
		//U_batt = 3.366 - (0.001*I_load) - ((0.0076*getQmax()) / (getQmax() - Qdt))*(I_load + Qdt) + 0.26422*(exp(-2.65487*Qdt));
		U_batt = 1.164*U_battnom - Rin*I_batt - (((0.076*getQmax()) / (getQmax() - Qdt))*(I_batt + Qdt)) + 3.5653*(exp(-26.5487*Qdt));
		I_load = U_batt / Rload;
		//U_batt = 3.366 - (((0.0076*getQmax()) / (getQmax() - Qdt))*(I_load + Qdt)) + 0.26422*exp(-26.5487*Qdt);
		//U_batt = 1.164*U_battnom - 0.01*I_load - (0.0076*getQmax() * 2 * I_load) / (getQmax() - Qdt) + 3.5653*exp(-0.12283*Qdt);
		file << t << "\t" << hsnapxa << "\t" << Qt << "\t" << soc << "\t"<<U_batt<<"\t" << I_batt << "\t" << getQmax()<<endl;

	}

file.close();

	//return dungluonghientai;    // ko co khi chay se chay voi tat ca cac gia tri cua lan chay truoc!!!!! 
						//(dungluongmax=dungluonghientai --> de thuc hien vong tieptheo)
};

double Battery::thoigianconlai()
{
	cout << "thoi gian con lai cua pin:" << endl;
	double thoigianconlai = Q_rated / I_load;
	cout << thoigianconlai << endl;
	return thoigianconlai;
};



Battery::Battery()
{
}


Battery::~Battery()
{
}
