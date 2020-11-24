#pragma once


#include <iostream>
#include <array>
#include <condition_variable>
#include <cstdio>
#include <stdexcept>

#include "ga.h"
#include "../wsnsim/lib/utilities.h"
#include "../wsnsim/test/test_phuong_phd_scenario1.h"


using namespace std;
using namespace wsn;

namespace the_test = test_phuong_phd_scenario1;



const char* exec_cmd = "scenario1.exe -r";

std::string exec_with_output(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}



class ga_scenario1 : public ga<the_test::network_schedule> {
public:
	ga_scenario1() {
		srand((unsigned int)time(NULL));
	}

	individual initial_individual() override {
		individual x;
//		x.init_default();
		x.set("0+ 10.0015 24.0011+ 61.3609 64.0242+ 81.6472 95.4332+ 147.525 175.989+ 203.403 205.992+ 226.942 237.396+ 253.751 310.793+ 327.068 416.823+ 763.84 1440+");
		return move(x);
	}

	individual mate(const individual& idv1, const individual& idv2) override {
		struct combined_interval {
			double start;
			bool i1active;
			bool i2active;
		};

		vector<combined_interval> combined;
		for (unsigned int i1 = 0, i2 = 0; i1 < idv1.node1.size() && i2 < idv2.node1.size();) {
			int idv1_smaller;

			if (i1 == idv1.node1.size()) idv1_smaller = -1;
			else if (i2 == idv2.node1.size()) idv1_smaller = 1;
			else if (idv1.node1[i1].start < idv2.node1[i2].start) idv1_smaller = 1;
			else if (idv1.node1[i1].start > idv2.node1[i2].start) idv1_smaller = -1;
			else idv1_smaller = 0;

			combined_interval t;
			if (idv1_smaller == 1) {
				t.start = idv1.node1[i1].start;
				t.i1active = idv1.node1[i1].active;
				t.i2active = idv2.node1[i2].active;
				i1++;
			}
			else if (idv1_smaller == -1) {
				t.start = idv2.node1[i2].start;
				t.i1active = idv1.node1[i1].active;
				t.i2active = idv2.node1[i2].active;
				i2++;
			}
			else {
				t.start = idv1.node1[i1].start;
				t.i1active = idv1.node1[i1].active;
				t.i2active = idv2.node1[i2].active;
				i1++;
				i2++;
			}

			combined.push_back(t);
		}

		individual x;

		for (unsigned int i = 0; i < combined.size(); i++) {
			auto t1 = combined[i];

			individual::interval t2;
			t2.start = t1.start;
			if (t1.i1active == t1.i2active) t2.active = t1.i1active;
			else {
				int r = rand() % 100;
				t2.active = r < 50 ? t1.i1active : t1.i2active;
			}

			if (x.node1.size() == 0 || t2.active != x.node1.back().active)
				x.node1.push_back(t2);
			else if (i == combined.size() - 1) {
				t2.active = !t2.active;
				x.node1.push_back(t2);
			}
		}

		for (unsigned int i = 0; i < x.node1.size() - 1; ) {
			int r = rand() % 100;

			if (r < 30) i++;	// no changes
			else if (r < 60) {	// move
				if (i > 0) {
					double rr = ((rand() % 20000) / 10000. - 1.) * .9;
					if (rr < 0.) x.node1[i].start += rr * (x.node1[i].start - x.node1[i - 1].start);
					else x.node1[i].start += rr * (x.node1[i + 1].start - x.node1[i].start);
				}
				i++;
			}
			else if (r < 80) {	// new sub interval
				double rr1 = (rand() % 10000) / 10000. * .8 + .1,
					rr2 = rr1;
				while (rr2 == rr1) rr2 = (rand() % 10000) / 10000. * .8 + .1;
				if (rr2 < rr1) swap(rr2, rr1);

				double s1 = rr1 * (x.node1[i + 1].start - x.node1[i].start) + x.node1[i].start,
					s2 = rr2 * (x.node1[i + 1].start - x.node1[i].start) + x.node1[i].start;

				individual::interval t1, t2;
				t1.active = !x.node1[i].active;
				t2.active = x.node1[i].active;
				t1.start = s1;
				t2.start = s2;

				x.node1.insert(x.node1.begin() + i + 1, { t1, t2 });
				i += 3;
			}
			else {	// removal
				if (x.node1.size() > 2) {
					if (i == 0) {
						x.node1[0].active = x.node1[1].active;
						x.node1.erase(x.node1.begin() + 1);
					}
					else if (i < x.node1.size() - 2) {
						x.node1.erase(x.node1.begin() + i, x.node1.begin() + i + 2);
					}
					else {
						x.node1.erase(x.node1.begin() + i);
						x.node1.back().active = !x.node1.back().active;
					}
				}
				i++;
			}
		}

		// remove short intervals
		for (unsigned int i = 0; i < x.node1.size() - 1; i++) {
			if (x.node1[i + 1].start - x.node1[i].start < 5) {
				if (i == 0) {
					x.node1[0].active = x.node1[1].active;
					x.node1.erase(x.node1.begin() + 1);
				}
				else if (i < x.node1.size() - 2) {
					x.node1.erase(x.node1.begin() + i, x.node1.begin() + i + 2);
				}
				else {
					x.node1.erase(x.node1.begin() + i);
					x.node1.back().active = !x.node1.back().active;
				}
			}
		}

		return move(x);
	}

	double eval_fitness(const individual& idv) override {
		idv.save();
		exec_with_output(exec_cmd);

		double fitness;
		ifstream file(the_test::filename_output, ios::binary);
		if (!file.read((char*)&fitness, sizeof(fitness)))
			return 1.e10;

		return fitness;
	}

	string individual2string(const individual& idv) const override {
		string s = kutils::formatstr("%d intervals: ", idv.node1.size());
		for (auto& i : idv.node1) {
			s += i.active ? kutils::formatstr("%g+ ", i.start) : kutils::formatstr("%g ", i.start);
		}
		return s;
	}
};

