#pragma once
#include <unordered_map>
#include <string>
#include <random>
#include <vector>

namespace exob {

class ProbabilityMap {
public:
    void add(const std::string& key, int weight) 
    { 
        if (weight <= 0) return;
        weights_[key] += weight; 
    }

    std::string pick(std::mt19937& rng) const 
    {
        int total = 0; for (auto &p: weights_) total += p.second;
        if (total <= 0) return {};
        std::uniform_int_distribution<int> dist(1, total);
        int v = dist(rng);
        for (auto &p: weights_) {
            v -= p.second;
            if (v <= 0) return p.first;
        }
        return weights_.begin()->first;
    }
    const std::unordered_map<std::string,int>& weights() const 
    { 
        return weights_; 
    }
private:
    std::unordered_map<std::string,int> weights_;
};

} // namespace exob
