#pragma once

#include "wsnsim.h"
#include <limits>


namespace wsn::power {


	class none : public basic_power {
	public:
		double get_max_power() const override {
			return 0.;
		}
	};


	class grid : public basic_power {
	public:
		double get_max_power() const override {
			return std::numeric_limits<double>::max();
		}
	};


	class solar : public basic_power {
	protected:
		double area = 5e-4,
			efficiency = .7;

	public:
		void set_area(double _area) {
			area = _area;
		}

		void set_efficiency(double _eff) {
			efficiency = _eff;
		}

		double get_max_power() const override {
			auto world = get_world();
			auto frame = world->get_reference_frame();

			auto node = get_node();
			auto loc = node->get_location();

			double lat, lon, alt;
			frame.local2lla(loc.x, loc.y, loc.z, lat, lon, alt);

			double rad = sun::get_radiation_power(lat, lon, frame.get_timezone(), world->get_clock().reference_now());
			return rad * area*efficiency;
		}
	};

}