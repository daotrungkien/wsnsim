#pragma once

#include "wsnsim.h"


namespace wsn::battery {



	class none : public basic_battery {
	public:
		bool charge(double T) override {
			fire(event_charge, T);
			return true;
		}

		bool consume(double T) override {
			fire(event_consume, T);
			return true;
		}

		double get_norminal_voltage() const override {
			return 1.5;
		}

		double get_capacity() const override {
			return .5;
		}

		double get_max_capacity() const override {
			return 1.;
		}
	};



	class linear : public basic_battery {
	protected:
		double minl, maxl, level;
		double charge_rate, consume_rate;

	public:
		linear() {
			minl = 0;
			maxl = 100;
			level = maxl;
			charge_rate = consume_rate = 1.;
		}

		bool charge(double T) override {
			if (level >= maxl) return false;

			auto power = get_node()->get_power();
			double effective_rate = std::min(charge_rate, power->get_max_power());

			double last_level = level;
			level += T * effective_rate;
			if (level == last_level) return false;

			if (level >= maxl) {
				level = maxl;
				fire(event_charge, level - last_level);
				fire(event_full);
				return true;
			}

			fire(event_charge, level - last_level);
			return true;
		}

		bool consume(double T) override {
			if (level <= minl) return false;

			double last_level = level;
			level -= T * consume_rate;
			if (level == last_level) return false;

			if (level <= minl) {
				level = minl;
				fire(event_consume, last_level - level);
				fire(event_empty);
				return false;
			}

			fire(event_consume, last_level - level);
			return true;
		}

		double get_norminal_voltage() const override {
			return 1.5;
		}

		double get_capacity() const override {
			return level;
		}

		double get_max_capacity() const override {
			return maxl;
		}



		void set_min_level(double _minl) {
			minl = _minl;
			if (level < minl) level = minl;
		}

		void set_max_level(double _maxl) {
			maxl = _maxl;
			if (level > maxl) level = maxl;
		}

		double get_min_level() const {
			return minl;
		}

		double get_max_level() const {
			return maxl;
		}

		void set_level(double l) {
			level = l;
		}

		void set_rates(double charge, double consume) {
			charge_rate = charge;
			consume_rate = consume;
		}

		double get_charge_rate() const {
			return charge_rate;
		}

		double get_consume_rate() const {
			return consume_rate;
		}
	};



	class chemical : public basic_battery {
	public:
		enum battery_type {
			lithium,
			nikel_cadminum,
			nikel_mh
		};

	protected:
		// parameters
		battery_type type;
		double R_load, U_battnorm, Q_rated, dt;

		// states
		double Qdt, Qmax;

	public:
		chemical() {
			type = lithium;
			set_norminal_voltage(1.5);
			set_load(1);
			set_rated_capacity(6500);
			dt = 0.01;

			Qdt = 0;
		}

		void set_load(double r) {
			R_load = r;
		}

		double get_load() const {
			return R_load;
		}

		void set_norminal_voltage(double nv) {
			U_battnorm = nv;
		}

		double get_norminal_voltage() const override {
			return U_battnorm;
		}

		void set_rated_capacity(double c) {
			Q_rated = c;

			switch (type) {
			case lithium:
				Qmax = Q_rated;
				break;
			case nikel_cadminum:
				Qmax = 1.1364 * Q_rated;
				break;
			case nikel_mh:
				Qmax = 1.07692 * Q_rated;
				break;
			}
		}

		double set_rated_capacity() const {
			return Q_rated;
		}

		double get_max_capacity() const override {
			return Qmax;
		}

		double get_capacity() const override {
			return Qmax - Qdt;
		}

		void set_sampling_time(double st) {
			dt = st;
		}

		double get_sampling_time() const {
			return dt;
		}

		double get_soc() const override {
			return 1. - Qdt / Qmax;
		}


		bool charge(double T) override {
			return evolve(false, T);
		}

		bool consume(double T) override {
			return evolve(true, T);
		}

	protected:
		bool evolve(bool consuming, double T) {
			if ((!consuming && Qdt <= 0.) || (consuming && Qdt >= Qmax)) return false;

			double last_Qdt = Qdt;

			int consuming_sign = consuming ? 1 : -1;
			double I_batt, U_batt,
				I_load = U_battnorm / R_load,
				Rin = (U_battnorm / Q_rated) / 100,
				rate, effective_rate;

			auto power = get_node()->get_power();

			for (double t = 0; t < T; t += dt) {
				rate = consuming_sign * I_load / 3600;
				effective_rate = consuming ? rate : std::min(rate, power->get_max_power());
				Qdt += rate * dt;

				if ((Qdt >= Qmax) || (Qdt <= 0.)) break;

				I_batt = consuming_sign * I_load;
				U_batt = 1.164*U_battnorm - Rin * I_batt - (((0.076*Qmax) / (Qmax - Qdt))*(I_batt + Qdt)) + 3.5653*(exp(-26.5487*Qdt));
				I_load = U_batt / R_load;
			}

			if (Qdt == last_Qdt) return false;

			if (consuming) {
				if (Qdt >= Qmax) {
					Qdt = Qmax;
					fire(event_consume, Qdt - last_Qdt);
					fire(event_empty);
					return false;
				}

				fire(event_consume, Qdt - last_Qdt);
				return true;
			}
			else {
				if (Qdt <= 0.) {
					Qdt = 0.;
					fire(event_charge, last_Qdt - Qdt);
					fire(event_full);
					return true;
				}

				fire(event_charge, last_Qdt - Qdt);
				return true;
			}
		}
	};


}