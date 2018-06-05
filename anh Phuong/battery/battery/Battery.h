#pragma once

class Battery
{
protected:
	double U_battnom,U_batt,I_batt, Rload, I_load;
	double Q_rated;
	double Qdt=0;
	int Tm;// thoi gian mo phong
	double Tstep;// thoi gian buoc tich phan
public:
	Battery();
	virtual ~Battery();
	void nhapthongsodau();
	void phantrampin();
	virtual double getQmax()=0;
	virtual double getEcharge() = 0;
	virtual double getEdischarge() = 0;
	virtual double thoigianconlai();
};