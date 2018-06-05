#pragma once

#include "wsnsim.h"



namespace wsn::sensor {



	class none : public basic_sensor {
	protected:
		void compute_value(std::any& value) const override {
			value = -1.;
		}
	};




	class with_ambient : public basic_sensor {
	protected:
		std::weak_ptr<basic_ambient> ambient;

		void compute_value(std::any& value) const override {
			auto node = get_node();
			assert(node);

			auto am = ambient.lock();
			assert(am);

			am->get_value(node->get_location(), value);
		}

	public:
		void set_ambient(std::shared_ptr<basic_ambient> _ambient) {
			ambient = _ambient;
		}
	};



	template <typename wrapped_sensor_type>
	class with_noise : public wrapped_sensor_type {
	protected:
		struct noise_info {
			uint key;
			std::shared_ptr<noise::basic_noise> noise;
		};

		std::list<noise_info> noises;

		void compute_value(std::any& value) const override {
			std::any v;
			wrapped_sensor_type::compute_value(v);

			double dv = std::any_cast<double>(v);
			for (auto& n : noises) {
				dv += n.noise->get_value();
			}

			value = dv;
		}

	public:
		uint add_noise(std::shared_ptr<noise::basic_noise> noise) {
			noise_info inf{ unique_id(), noise };
			noises.push_back(inf);
			return inf.key;
		}

		std::shared_ptr<noise::basic_noise> find_noise(uint key) const {
			auto itr = std::find_if(noises.begin(), noises.end(), [key](auto& e) {
				return e.key == key;
			});

			return (itr == noises.end()) ? nullptr : itr->noise;
		}

		void remove_noise(uint key) {
			auto itr = std::find_if(noises.begin(), noises.end(), [key](auto& e) {
				return e.key == key;
			});

			if (itr != noises.end())
				noises.erase(itr);
		}
	};


}