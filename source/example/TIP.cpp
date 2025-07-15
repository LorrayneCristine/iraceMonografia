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

TIP::TIP(const std::string &filename, int mode_, int movType_)
    : mode(mode_), movType(movType_)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[TIP] Falha ao abrir: " << filename << std::endl;
        std::exit(1);
    }

    if (mode == 1) {
        //
        // MODO 1: sequência de operações
        //
        file >> magazineSize;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string line;
        std::getline(file, line);
        std::unordered_set<int> toolSet;
        std::istringstream iss(line);
        int x;
        while (iss >> x) {
            toolSequence.push_back(x);
            toolSet.insert(x);
        }

        uniqueTools.assign(toolSet.begin(), toolSet.end());
        std::sort(uniqueTools.begin(), uniqueTools.end());
        for (int i = 0; i < (int)uniqueTools.size(); ++i)
            toolToIndex[uniqueTools[i]] = i;
        numTools = uniqueTools.size();
    }
    else if(mode == 2 || mode == 3){
        //
        // MODO 2: matriz de frequência
        //
        file >> numTools >> magazineSize;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string dummy;
        std::getline(file, dummy); // solucao
        std::getline(file, dummy); // custo

        frequencyMatrix.assign(numTools, std::vector<int>(numTools));
        for (int i = 0; i < numTools; ++i) {
            std::getline(file, dummy);
            std::istringstream fss(dummy);
            for (int j = 0; j < numTools; ++j)
                fss >> frequencyMatrix[i][j];
        }

        uniqueTools.resize(numTools);
        for (int i = 0; i < numTools; ++i)
            uniqueTools[i] = i + 1;
    }
}

TIP::~TIP() {}

solTIP TIP::construction() {
    // chamada padrão → pode usar versão com ID se quiser
    return construction(-1);  // ou -1 para indicar "sem heurística"
}

solTIP TIP::construction(int replicaID) {
    solTIP sol;

    bool usarHeuristica = heuristicReplicas.find(replicaID) != heuristicReplicas.end();

    if (usarHeuristica) {
        //std::cout << "[DEBUG] Réplica " << replicaID << " usando HEURÍSTICA\n";
        sol = greedyConstructionFreq();
    } else {
        //std::cout << "[DEBUG] Réplica " << replicaID << " usando ALEATÓRIA\n";
        sol.permutation.resize(numTools);
        for (int i = 0; i < numTools; ++i)
            sol.permutation[i] = i;
        std::shuffle(sol.permutation.begin(), sol.permutation.end(), rndEngine);
    }

    sol.evalSol = evaluate(sol);
    return sol;
}







solTIP TIP::greedyConstructionFreq() {
    solTIP sol;
    sol.permutation.clear();
    std::vector<bool> used(numTools, false);

    using Pair = std::pair<int, int>;
    std::vector<std::tuple<int, Pair>> freqPairs;

    for (int i = 0; i < numTools; ++i) {
        for (int j = i + 1; j < numTools; ++j) {
            int freq = frequencyMatrix[i][j];
            if (freq > 0)
                freqPairs.emplace_back(freq, std::make_pair(i, j));
        }
    }

    std::sort(freqPairs.rbegin(), freqPairs.rend()); // maior frequência primeiro

    for (const auto& [freq, pair] : freqPairs) {
        int i = pair.first;
        int j = pair.second;

        if (!used[i]) {
            sol.permutation.push_back(i);
            used[i] = true;
        }
        if (!used[j]) {
            sol.permutation.push_back(j);
            used[j] = true;
        }

        if ((int)sol.permutation.size() == numTools)
            break;
    }

    // Adiciona qualquer ferramenta que não apareceu ainda
    for (int i = 0; i < numTools; ++i) {
        if (!used[i]) {
            sol.permutation.push_back(i);
        }
    }

    return sol;
}



solTIP TIP::neighbor(solTIP sol) {
    switch (movType) {
        case 1: return swapNeighbor(sol);
        case 2: return insertionNeighbor(sol);
        case 3: return twoOptNeighbor(sol);
        case 4: return randomNeighbor(sol);
        default: return swapNeighbor(sol);
    }
}



// TIP.cpp

// ---- operação 1: swap ----
solTIP TIP::swapNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    while (i == j) j = dist(rndEngine);
    // DEBUG:
   //std::cout << "[TIP] usando swapNeighbor: i=" << i << " j=" << j << "\n";
    std::swap(sol.permutation[i], sol.permutation[j]);
    sol.evalSol = evaluate(sol);
    return sol;
}

// ---- operação 2: insertion ----
solTIP TIP::insertionNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    while (i == j) j = dist(rndEngine);
    // DEBUG:
    //std::cout << "[TIP] usando insertionNeighbor: remove pos=" << i
    //          << " e insere em pos=" << j << "\n";
    int elem = sol.permutation[i];
    if (i < j) {
        for (int k = i; k < j; ++k)
            sol.permutation[k] = sol.permutation[k+1];
        sol.permutation[j] = elem;
    } else {
        for (int k = i; k > j; --k)
            sol.permutation[k] = sol.permutation[k-1];
        sol.permutation[j] = elem;
    }
    sol.evalSol = evaluate(sol);
    return sol;
}

// ---- operação 3: 2-opt (reverte o trecho [i..j]) ----
solTIP TIP::twoOptNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    if (i > j) std::swap(i, j);
    if (i == j) {
        j = (i + 1) % numTools;
        if (i > j) std::swap(i, j);
    }
    // DEBUG:
   // std::cout << "[TIP] usando twoOptNeighbor: revertendo trecho ["
    //          << i << ".." << j << "]\n";
    std::reverse(sol.permutation.begin() + i,
                 sol.permutation.begin() + j + 1);
    sol.evalSol = evaluate(sol);
    return sol;
}


// ---- operação 4: random (swap, insertion ou 2-opt aleatório) ----
solTIP TIP::randomNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(1, 3);  // 1=swap, 2=insertion, 3=2-opt
    int choice = dist(rndEngine);
    
    switch (choice) {
        case 1:
            return swapNeighbor(sol);
        case 2:
            return insertionNeighbor(sol);
        case 3:
            return twoOptNeighbor(sol);
        default:
            return sol;  // fallback (não deve acontecer)
    }
}



double TIP::evaluate(solTIP sol) {
    if (mode == 1) {
        //
        // MODO 1: sequência de operações
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
    std::vector<int> mag(magazineSize, -1);
    for (int i = 0; i < (int)sol.permutation.size(); ++i) {
        int toolIdx = sol.permutation[i];
        int toolID  = uniqueTools[toolIdx];
        mag[i]      = toolID;
    }
    return mag;
}

// (os demais auxiliares de impressão / contagem de término permanecem iguais)

void TIP::setHeuristicReplicas(const std::unordered_set<int>& replicas) {
    heuristicReplicas = replicas;
}
