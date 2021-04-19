#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <random>
#include <fstream>

#include "../wsnsim/lib/utilities.h"



class ga_logging {
protected:
	std::ofstream file;

public:
	ga_logging() {
		auto filename = kutils::formatstr("ga-log-%s.txt", kutils::format_time(time(nullptr), "%Y%m%d%H%M").c_str());
		file.open(filename, std::ios::out);

		if (!file) std::cout << "GA log file creation failed: " << filename << std::endl;
	}

	void log(std::string msg) {
		std::cout << msg << std::endl;
		file << msg << std::endl;
	}
};

extern ga_logging ga_logger;



// genetic algorithm

template <typename _individual>
class ga {
public:
	using individual = _individual;

protected:
	int population_size = 20;
	double selection_rate = .2;
	double mate_elite_rate = .5;

	int generation = 0, stall_generations = 0;
	std::vector<individual> population, new_population;
	std::vector<double> previous_fitness, current_fitness;

	std::random_device rand_dev;
	std::mt19937 rand_gen;
	std::piecewise_linear_distribution<double> mate_rand_dist;

public:
	virtual std::vector<individual> initial_population() = 0;
	virtual bool identical(const individual& idv1, const individual& idv2) { return false; }
	virtual individual mate(const individual& idv1, const individual& idv2) = 0;
	virtual double eval_fitness(const individual& idv) = 0;

	virtual std::string individual2string(const individual& idv) const {
		return "[individual2string() not implemented]";
	}

	virtual bool check_termination() const {
		return generation == 1000 || stall_generations == 100;
	}

	virtual void post_process() const {
		auto minf = std::min_element(current_fitness.begin(), current_fitness.end());
		int minf_idx = minf - current_fitness.begin();

		ga_logger.log(kutils::formatstr(
			"- Fitness (min, mean, max): %g, %g, %g\n"
			"- Min gnome: %s",
			*minf,
			std::accumulate(current_fitness.begin(), current_fitness.end(), 0.) / population.size(),
			*std::max_element(current_fitness.begin(), current_fitness.end()),
			individual2string(population[minf_idx]).c_str()));
	}

public:
	ga() {
		rand_gen = std::mt19937(rand_dev());
		mate_rand_dist = std::piecewise_linear_distribution<double>(100, 0., 1., [](double x) { return 1. - x * x; });
	}

	void set_population_size(int n) {
		population_size = n;
	}

	int get_population_size() const {
		return population_size;
	}

	void set_selection_rate(double r) {
		selection_rate = r;
	}

	double get_selection_rate() const {
		return selection_rate;
	}


	std::pair<individual, double> run() {
		population = initial_population();

		std::vector<std::pair<int, double>> fitness_info;

		stall_generations = 0;

		for (generation = 1; !check_termination(); generation++) {
			unsigned int true_population_size = population.size();

			previous_fitness = current_fitness;

			ga_logger.log(kutils::formatstr("Epoch #%d:", generation));

			for (unsigned int i = 0; i < true_population_size; i++) {
				ga_logger.log(kutils::formatstr("- Gnome #%d: %s", i, individual2string(population[i]).c_str()));
			}
			
			fitness_info.resize(true_population_size);
			current_fitness.resize(true_population_size);

			// evaluate fitness score for the population
			for (unsigned int i = 0; i < true_population_size; i++) {
				double f = eval_fitness(population[i]);
				fitness_info[i] = std::make_pair(i, f);
				current_fitness[i] = f;

				std::cout << "#" << i << "=" << f << " ";
			}
			std::cout << std::endl;

			if (generation > 1) {
				auto prev_minf = std::min_element(previous_fitness.begin(), previous_fitness.end());
				auto minf = std::min_element(current_fitness.begin(), current_fitness.end());
				if (*prev_minf <= *minf)
					stall_generations++;
				else stall_generations = 0;
			}

			post_process();

			unsigned int selection_size = (unsigned int)(selection_rate * population_size);
			if (selection_size > true_population_size) selection_size = true_population_size;

			// sort individuals by fitness
			std::sort(fitness_info.begin(), fitness_info.end(), [](auto& a, auto& b) {
				return a.second < b.second;
			});

			// mutation and crossover
			for (unsigned int i = 0; i < population_size - selection_size; i++) {
				int f1 = (int)(mate_rand_dist(rand_gen) * true_population_size),
					f2 = (int)(mate_rand_dist(rand_gen) * true_population_size);
				new_population.push_back(mate(population[f1], population[f2]));
			}

			// selection
			for (unsigned int i = 0; i < selection_size; i++) {
				int fi = fitness_info[i].first;
				new_population.insert(new_population.begin(), std::move(population[fi]));
			}
			population.clear();

			// filter duplicate individuals
			for (unsigned int i = 0; i < new_population.size() - 1; i++) {
				for (unsigned int j = i + 1; j < new_population.size(); j++) {
					if (identical(new_population[i], new_population[j])) {
						new_population.erase(new_population.begin() + j);
						j--;
					}
				}
			}

			population.swap(new_population);
		}

		return std::make_pair(population[0], fitness_info[0].first);
	}
};
