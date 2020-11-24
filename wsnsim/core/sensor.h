#pragma once

#include "wsnsim.h"



namespace wsn::sensor {



	class none : public basic_sensor {
	protected:
		void compute_value(std::any& value) const override {
			value = -1.;
		}
	};


	template <typename wrapped_sensor_type>
	class with_periodical : public wrapped_sensor_type {
	protected:
		std::chrono::duration<double> sampling_time;
		bool active = true;

	public:
		template <typename _Rep, typename _Period>
		void set_sampling_time(std::chrono::duration<_Rep, _Period> st) {
			sampling_time = chrono::duration_cast<chrono::duration<double>>(st);
		}

		bool is_active() const {
			return active;
		}

		void set_active(bool a) {
			active = a;
		}

		void init() override {
			wrapped_sensor_type::init();

			on_self(entity::event_first_start, [this](event& ev) {
				timer(sampling_time, true, [this](event& ev) {
					if (active) wrapped_sensor_type::measure();
				});
			});
		}
	};


	template <typename wrapped_sensor_type>
	class with_ambient : public wrapped_sensor_type {
	protected:
		std::weak_ptr<basic_ambient> ambient;

		void compute_value(std::any& value) const override {
			auto node = this->get_node();
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