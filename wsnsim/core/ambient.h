#pragma once

#include "wsnsim.h"




namespace wsn {




class constant_real_value_ambient : public basic_ambient {
protected:
	double cvalue;

public:
	constant_real_value_ambient(double _value)
		: cvalue(_value)
	{
	}

	void set_value(double _value) {
		cvalue = _value;
	}

	void get_value(const location& loc, std::any& value) override {
		value = cvalue;
	}
};




class smooth_real_value_ambient : public basic_ambient {
protected:
	double freq, mean;

public:
	smooth_real_value_ambient(double _mean = 0., double _freq = 1.)
		: mean(_mean), freq(_freq)
	{
	}

	void get_value(const location& loc, std::any& value) override {
		double t = get_world_clock_time();
		value = mean + std::sin(t * 2 * M_PI / freq + loc.x / 100. + loc.y / 120. + loc.z/170.);
	}
};


}