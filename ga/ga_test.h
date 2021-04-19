#pragma once

#include <array>


#include "../wsnsim/ga/ga.h"
#include "../wsnsim/lib/utilities.h"


class ga_test : public ga<std::array<double, 2>> {
public:
	std::vector<individual> initial_population() override {
		std::vector<individual> p;
		
		individual a;
		a[0] = (rand() % 1000 / 500. - 1.) * 10;
		a[1] = (rand() % 1000 / 500. - 1.) * 10;
		p.push_back(a);

		return move(p);
	}

	individual mate(const individual& idv1, const individual& idv2) override {
		individual a;

		for (int i = 0; i < 2; i++) {
			int t = rand() % 100;
			if (t < 30) a[i] = idv1[i];
			else if (t < 60) a[i] = idv2[i];
			else a[i] = (rand() % 1000 / 500. - 1.) * 10;
		}

		return move(a);
	}

	double eval_fitness(const individual& idv) override {
		return pow(idv[0] - 3, 2) + pow(idv[1] - 2, 2);
	}

	std::string individual2string(const individual& idv) const override {
		return kutils::formatstr("[%.3lf, %.3lf]", idv[0], idv[1]);
	}
};
