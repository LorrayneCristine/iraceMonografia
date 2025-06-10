// TIP.cpp

#include "TIP.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>
#include <mutex>
#include <sstream>
#include <thread>
#include <limits>
#include <iomanip>

// Gerador de números aleatórios, um por thread
thread_local std::mt19937 TIP::rndEngine{std::random_device{}()};

TIP::TIP(const std::string &filename, bool useFreqMode)
  : useFreq(useFreqMode)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[TIP] Falha ao abrir: " << filename << std::endl;
        std::exit(1);
    }

    if (!useFreq) {
        //
        // MODO 1: sequência de operações (sem usar distanceMatrix)
        //

        // 1) lê magazineSize
        file >> magazineSize;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // 2) lê a linha inteira com a sequência de ferramentas
        std::string line;
        std::getline(file, line);

        // 3) parseia e armazena em toolSequence, e constrói o conjunto de ferramentas únicas
        std::unordered_set<int> toolSet;
        std::istringstream iss(line);
        int x;
        while (iss >> x) {
            toolSequence.push_back(x);
            toolSet.insert(x);
        }

        // 4) monta vetor uniqueTools e mapeamento
        uniqueTools.assign(toolSet.begin(), toolSet.end());
        std::sort(uniqueTools.begin(), uniqueTools.end());
        for (int i = 0; i < (int)uniqueTools.size(); ++i)
            toolToIndex[uniqueTools[i]] = i;
        numTools = uniqueTools.size();

    }
    else {
        //
        // MODO 2: matriz de frequência
        //

        // 1) lê numTools e magazineSize
        file >> numTools >> magazineSize;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // 2) ignora a linha de solução (magazine)
        std::string dummy;
        std::getline(file, dummy);

        // 3) ignora o valor da solução
        std::getline(file, dummy);

        // 4) lê a matriz de frequência simétrica
        frequencyMatrix.assign(numTools, std::vector<int>(numTools));
        for (int i = 0; i < numTools; ++i) {
            std::getline(file, dummy);
            std::istringstream fss(dummy);
            for (int j = 0; j < numTools; ++j)
                fss >> frequencyMatrix[i][j];
        }

        // 5) uniqueTools = {1,2,…,numTools}
        uniqueTools.resize(numTools);
        for (int i = 0; i < numTools; ++i)
            uniqueTools[i] = i + 1;
    }
}

TIP::~TIP() {}

solTIP TIP::construction() {
    solTIP sol;
    sol.permutation.resize(numTools);
    for (int i = 0; i < numTools; ++i)
        sol.permutation[i] = i;
    std::shuffle(sol.permutation.begin(), sol.permutation.end(), rndEngine);
    sol.evalSol = evaluate(sol);
    return sol;
}

solTIP TIP::neighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    while (i == j) j = dist(rndEngine);

    std::swap(sol.permutation[i], sol.permutation[j]);
    sol.evalSol = evaluate(sol);
    return sol;
}

double TIP::evaluate(solTIP sol) {
    if (!useFreq) {
        //
        // MODO 1: sequência de operações (recalcula distâncias diretamente)
        //
        std::unordered_map<int,int> pos;
        for (int slot = 0; slot < (int)sol.permutation.size(); ++slot) {
            int toolID = uniqueTools[ sol.permutation[slot] ];
            pos[toolID] = slot;
        }

        double cost = 0.0;
        for (size_t k = 0; k + 1 < toolSequence.size(); ++k) {
            int t1 = toolSequence[k], t2 = toolSequence[k+1];
            int s1 = pos[t1], s2 = pos[t2];
            int delta = std::abs(s1 - s2);
            int d = std::min(delta, magazineSize - delta);
            cost += d;
        }
        return cost;
    }
    else {
        //
        // MODO 2: matriz de frequência
        //
        std::vector<int> pos(numTools);
        for (int slot = 0; slot < (int)sol.permutation.size(); ++slot)
            pos[ sol.permutation[slot] ] = slot;

        double cost = 0.0;
        for (int i = 0; i < numTools; ++i) {
            for (int j = i + 1; j < numTools; ++j) {
                int freq = frequencyMatrix[i][j];
                if (freq == 0) continue;
                int delta = std::abs(pos[i] - pos[j]);
                int d = std::min(delta, magazineSize - delta);
                cost += double(freq) * d;
            }
        }
        return cost;
    }
}

solTIP TIP::getBestSol() {
    return bestSol;
}

std::vector<int> TIP::decodeSolution(const solTIP &sol) const {
    // -1 = slot vazio
    std::vector<int> mag(magazineSize, -1);
    for (int i = 0; i < (int)sol.permutation.size(); ++i) {
        int toolIdx = sol.permutation[i];
        int toolID  = uniqueTools[toolIdx];
        mag[i]      = toolID;
    }
    return mag;
}
