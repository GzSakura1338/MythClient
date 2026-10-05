#include "minecraft/utils/rotation/Random.h"

#include <random>

#include "core/Instance.h"

namespace nova::rotation {
namespace {

std::mt19937_64& Engine() {
    return Instance<std::mt19937_64>([] { return new std::mt19937_64(std::random_device{}()); });
}

}

double Random() {
    return std::uniform_real_distribution<double>(0.0, 1.0)(Engine());
}

double RandomRange(double min, double max) {
    return min + Random() * (max - min);
}

double Gaussian(double mean, double standardDeviation) {
    return std::normal_distribution<double>(mean, standardDeviation)(Engine());
}

}
