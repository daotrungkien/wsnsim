#pragma once

#include "wsnsim.h"



namespace wsn::noise {
	class basic_noise : public wsn_type {
	public:
		virtual double get_value() = 0;
	};


	class none : public basic_noise {
	public:
		double get_value() override {
			return 0.;
		}
	};


	class random : public basic_noise {
	protected:
		std::default_random_engine random_generator;
	public:
		random()
			: random_generator((uint)time(nullptr))
		{
		}
	};


	class gaussian : public random {
	protected:
		std::normal_distribution<double> dist;
	public:
		gaussian(double mean = 0., double deviation = 1.)
			: dist(mean, deviation)
		{
		}

		void set_noise(double mean, double deviation) {
			dist = std::normal_distribution<double>(mean, deviation);
		}

		double get_value() override {
			return dist(random_generator);
		}
	};
}

