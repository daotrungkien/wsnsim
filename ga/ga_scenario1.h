#pragma once


#include <iostream>
#include <array>
#include <condition_variable>
#include <cstdio>
#include <stdexcept>
#include <map>

#include "ga.h"
#include "../wsnsim/lib/utilities.h"
#include "../wsnsim/test/test_phuong_phd_scenario1.h"




using namespace std;
using namespace wsn;

namespace the_test = test_phuong_phd_scenario1;


const int fitness_max_cache = 5;



std::string exec_with_output(const string& cmd) {
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd.c_str(), "r"), _pclose);
	if (!pipe) throw std::runtime_error("popen() failed!");

	std::array<char, 128> buffer;
	std::string result;
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return move(result);
}



struct network_schedule_compare {
	bool operator()(const the_test::network_schedule& s1, const the_test::network_schedule& s2) const {
		for (uint i = 0; i < the_test::sensor_nodes; i++) {
			auto n1 = s1.node_schedules[i].size(),
				n2 = s2.node_schedules[i].size();
			if (n1 < n2) return true;
			if (n1 > n2) return false;

			for (uint j = 0; j < n1; j++) {
				auto c1 = s1.node_schedules[i][j],
					c2 = s2.node_schedules[i][j];
				if (c1.start < c2.start) return true;
				if (c1.start > c2.start) return false;
				if (c1.active < c2.active) return true;
				if (c1.active > c2.active) return false;
			}
		}

		return false;
	}
};





class ga_scenario1 : public ga<the_test::network_schedule> {
protected:
	double copy_rate = 0.9;

	std::uniform_real_distribution<double> oprt_rand_dist;
	map<individual, vector<double>, network_schedule_compare> fitness_cache;

	bool identical(const individual& idv1, const individual& idv2) override {
		for (uint i = 0; i < idv1.node_schedules.size(); i++) {
			if (idv1.node_schedules[i].size() != idv2.node_schedules[i].size()) return false;

			for (uint j = 0; j < idv1.node_schedules[i].size(); j++) {
				auto& a1 = idv1.node_schedules[i][j],
					& a2 = idv2.node_schedules[i][j];
				if (a1.active != a2.active || a1.start != a2.start) return false;
			}
		}

		return true;
	}

	virtual vector<the_test::network_schedule::interval> mate_node(const vector<the_test::network_schedule::interval>& idv1, const vector<the_test::network_schedule::interval>& idv2) = 0;

public:
	static vector<individual> init_population;


	ga_scenario1() {
		oprt_rand_dist = std::uniform_real_distribution<double>(0., 1.);
		srand((unsigned int)time(NULL));
	}

	void set_copy_rate(double r) {
		copy_rate = r;
	}

	vector<individual> initial_population() override {
		if (init_population.size() > 0) return init_population;

		vector<the_test::network_schedule> p;
		p.push_back(individual());
		return move(p);
	}

	individual mate(const individual& idv1, const individual& idv2) override {
		individual x;

		for (uint i = 0; i < x.node_schedules.size(); i++) {
			auto xi = mate_node(idv1.node_schedules[i], idv2.node_schedules[i]);
			x.set(i, xi);
		}

		return move(x);
	}

	
	double eval_fitness(const individual& idv) override {
		// check in the cache first...
		auto findres = fitness_cache.find(idv);
		if (findres != fitness_cache.end()) {
			auto& values = findres->second;
			if (values.size() >= fitness_max_cache) {
				double avg = 0.;
				for (uint i = 0; i < fitness_max_cache; i++)
					avg += values[i];
				avg /= fitness_max_cache;

				if (avg / values[0] <= 1.1) return values[0];
			}
		}
		
		// ...evaluate if neccessary!
		idv.save();

		double fitness = 1e10;
		string exec_cmd = kutils::formatstr("scenario1.exe -r -n %d", the_test::sensor_nodes);
		for (int i = 0; i < 5; i++) {
			exec_with_output(exec_cmd);

			double tmp_fitness;
			ifstream file(the_test::filename_output, ios::binary);
			if (file.read((char*)&tmp_fitness, sizeof(tmp_fitness))) {
				fitness = tmp_fitness;
				break;
			}

			cout << "-";
		}

		// save to the cache
		if (findres == fitness_cache.end()) {
			fitness_cache.insert(make_pair(idv, vector<double>{ fitness }));
			return fitness;
		}
		else {
			auto& values = findres->second;
			auto upper = upper_bound(values.begin(), values.end(), fitness);
			values.insert(upper, fitness);
			return *values.begin();
		}
	}

	string individual2string(const individual& idv) const override {
		string s;

		for (uint ni = 0; ni < idv.node_schedules.size(); ni++) {
			if (ni > 0) s += "+";
			s += to_string(idv.node_schedules[ni].size());
		}
		
		s += " intervals: { ";

		for (uint ni = 0; ni < idv.node_schedules.size(); ni++) {
			if (ni > 0) s += "; ";

			for (auto& i : idv.node_schedules[ni]) {
				s += i.active ? kutils::formatstr("%g+ ", i.start) : kutils::formatstr("%g ", i.start);
			}
		}

		s += "}";
		return s;
	}
};




class ga_scenario1_fixed : public ga_scenario1 {
protected:
	vector<the_test::network_schedule::interval> mate_node(const vector<the_test::network_schedule::interval>& idv1, const vector<the_test::network_schedule::interval>& idv2) override {
		assert(idv1.size() == idv2.size());

		vector<the_test::network_schedule::interval> x = idv1;

		for (uint i = 0; i < idv1.size(); i++) {
			double r = oprt_rand_dist(rand_gen);

			// crossover
			if (r < copy_rate) {
				int r = rand() % 100;
				x[i].active = (r < 50 ? idv1[i] : idv2[i]).active;
				continue;
			}

			// mutation
			x[i].active = !x[i].active;
		}

		return move(x);
	}

public:
	ga_scenario1_fixed(uint blocks = 24) {
		string s;

		double dt = (24. * 60.) / blocks;
		for (uint n = 0; n < the_test::sensor_nodes; n++) {
			if (n > 0) s += " ; ";

			for (uint i = 0; i <= blocks; i++) {
				if (i > 0) s += " ";
				s += kutils::formatstr("%g", i * dt);
				if (rand() % 2) s += "+";
			}
		}

		init_population.push_back(s.c_str());
	}
};



class ga_scenario1_variable : public ga_scenario1 {
protected:
	vector<the_test::network_schedule::interval> mate_node(const vector<the_test::network_schedule::interval>& idv1, const vector<the_test::network_schedule::interval>& idv2) override {
		struct combined_interval {
			double start;
			bool i1active;
			bool i2active;
		};

		vector<combined_interval> combined;
		for (unsigned int i1 = 0, i2 = 0; i1 < idv1.size() && i2 < idv2.size();) {
			int idv1_smaller;

			if (i1 == idv1.size()) idv1_smaller = -1;
			else if (i2 == idv2.size()) idv1_smaller = 1;
			else if (idv1[i1].start < idv2[i2].start) idv1_smaller = 1;
			else if (idv1[i1].start > idv2[i2].start) idv1_smaller = -1;
			else idv1_smaller = 0;

			combined_interval t;
			if (idv1_smaller == 1) {
				t.start = idv1[i1].start;
				t.i1active = idv1[i1].active;
				t.i2active = idv2[i2].active;
				i1++;
			}
			else if (idv1_smaller == -1) {
				t.start = idv2[i2].start;
				t.i1active = idv1[i1].active;
				t.i2active = idv2[i2].active;
				i2++;
			}
			else {
				t.start = idv1[i1].start;
				t.i1active = idv1[i1].active;
				t.i2active = idv2[i2].active;
				i1++;
				i2++;
			}

			combined.push_back(t);
		}

		vector<the_test::network_schedule::interval> x;

		for (uint i = 0; i < combined.size(); i++) {
			auto t1 = combined[i];

			individual::interval t2;
			t2.start = t1.start;
			if (t1.i1active == t1.i2active) t2.active = t1.i1active;
			else {
				int r = rand() % 100;
				t2.active = r < 50 ? t1.i1active : t1.i2active;
			}

			if (x.size() == 0 || t2.active != x.back().active)
				x.push_back(t2);
			else if (i == combined.size() - 1) {
				t2.active = !t2.active;
				x.push_back(t2);
			}
		}

		for (uint i = 0; i < x.size() - 1; ) {
			double r = oprt_rand_dist(rand_gen);

			if (r < copy_rate) {
				i++;	// no changes
				continue;
			}

			r = (r - copy_rate) / (1. - copy_rate);

			if (r < .33) {	// shift
				if (i > 0) {
					double rr = ((rand() % 20000) / 10000. - 1.) * .9;
					if (rr < 0.) x[i].start += rr * (x[i].start - x[i - 1].start);
					else x[i].start += rr * (x[i + 1].start - x[i].start);
				}
				i++;
			}
			else if (r < .67) {	// insert
				double rr1 = (rand() % 10000) / 10000. * .8 + .1,
					rr2 = rr1;
				while (rr2 == rr1) rr2 = (rand() % 10000) / 10000. * .8 + .1;
				if (rr2 < rr1) swap(rr2, rr1);

				double s1 = rr1 * (x[i + 1].start - x[i].start) + x[i].start,
					s2 = rr2 * (x[i + 1].start - x[i].start) + x[i].start;

				individual::interval t1, t2;
				t1.active = !x[i].active;
				t2.active = x[i].active;
				t1.start = s1;
				t2.start = s2;

				x.insert(x.begin() + i + 1, { t1, t2 });
				i += 3;
			}
			else {	// remove
				if (x.size() > 2) {
					if (i == 0) {
						x[0].active = x[1].active;
						x.erase(x.begin() + 1);
					}
					else if (i < x.size() - 2) {
						x.erase(x.begin() + i, x.begin() + i + 2);
					}
					else {
						x.erase(x.begin() + i);
						x.back().active = !x.back().active;
					}
				}
				i++;
			}
		}

		// remove short intervals
		for (uint i = 0; i < x.size() - 1; i++) {
			if (x[i + 1].start - x[i].start < 5) {
				if (i == 0) {
					x[0].active = x[1].active;
					x.erase(x.begin() + 1);
				}
				else if (i < x.size() - 2) {
					x.erase(x.begin() + i, x.begin() + i + 2);
				}
				else {
					x.erase(x.begin() + i);
					x.back().active = !x.back().active;
				}
			}
		}

		return move(x);
	}

public:
};