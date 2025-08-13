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

// -----------------------------------------------------------------------------
// Inicialização do gerador aleatório por thread (necessário para paralelismo)
// -----------------------------------------------------------------------------
thread_local std::mt19937 TIP::rndEngine{std::random_device{}()};

// -----------------------------------------------------------------------------
// Construtor da classe TIP
// Lê o arquivo da instância e prepara os dados de entrada
// -----------------------------------------------------------------------------
TIP::TIP(const std::string &filename, int mode_, int movType_)
    : mode(mode_), movType(movType_) {

    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[TIP] Falha ao abrir: " << filename << std::endl;
        std::exit(1);
    }

    if (mode == 1) {
        // ------------------------------------------------------------
        // MODO 1: leitura da sequência de operações
        // ------------------------------------------------------------
        file >> magazineSize; // número de slots
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

        // Ordena ferramentas únicas e cria o mapeamento
        uniqueTools.assign(toolSet.begin(), toolSet.end());
        std::sort(uniqueTools.begin(), uniqueTools.end());
        for (int i = 0; i < (int)uniqueTools.size(); ++i)
            toolToIndex[uniqueTools[i]] = i;

        numTools = uniqueTools.size();
    }
    else if (mode == 2 || mode == 3) {
        // ------------------------------------------------------------
        // MODO 2 ou 3: leitura da matriz de frequência
        // ------------------------------------------------------------
        file >> numTools >> magazineSize;
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        std::string dummy;
        std::getline(file, dummy); // linha da solução
        std::getline(file, dummy); // linha do custo

        frequencyMatrix.assign(numTools, std::vector<int>(numTools));
        for (int i = 0; i < numTools; ++i) {
            std::getline(file, dummy);
            std::istringstream fss(dummy);
            for (int j = 0; j < numTools; ++j)
                fss >> frequencyMatrix[i][j];
        }

        uniqueTools.resize(numTools);
        for (int i = 0; i < numTools; ++i)
            uniqueTools[i] = i + 1; // ferramentas de 1 a N
    }
}

// -----------------------------------------------------------------------------
// Destrutor
// -----------------------------------------------------------------------------
TIP::~TIP() {}

// -----------------------------------------------------------------------------
// Método padrão de construção (sem ID específico)
// -----------------------------------------------------------------------------
solTIP TIP::construction() {
    return construction(-1); // passa ID -1 para indicar ausência
}

// -----------------------------------------------------------------------------
// Método de construção de solução com identificação de réplica
// Usa heurística gulosa ou aleatória, dependendo do ID
// -----------------------------------------------------------------------------
solTIP TIP::construction(int replicaID) {
    solTIP sol;

    // Verifica se a réplica atual está marcada como heurística
    bool usarHeuristica = heuristicReplicas.find(replicaID) != heuristicReplicas.end();

    if (usarHeuristica) {
        // construção gulosa baseada na matriz de frequência
        sol = greedyConstructionFreq();
    } else {
        // construção totalmente aleatória
        sol.permutation.resize(numTools);
        for (int i = 0; i < numTools; ++i)
            sol.permutation[i] = i;

        std::shuffle(sol.permutation.begin(), sol.permutation.end(), rndEngine);
    }

    sol.evalSol = evaluate(sol); // avalia custo da solução
    return sol;
}

// -----------------------------------------------------------------------------
// Construção gulosa baseada em pares de maior frequência
// -----------------------------------------------------------------------------
solTIP TIP::greedyConstructionFreq() {
    solTIP sol;
    sol.permutation.clear();

    std::vector<bool> used(numTools, false);
    using Pair = std::pair<int, int>;
    std::vector<std::tuple<int, Pair>> freqPairs;

    // Cria lista de pares com frequência positiva
    for (int i = 0; i < numTools; ++i) {
        for (int j = i + 1; j < numTools; ++j) {
            int freq = frequencyMatrix[i][j];
            if (freq > 0)
                freqPairs.emplace_back(freq, std::make_pair(i, j));
        }
    }

    // Ordena pares por frequência decrescente
    std::sort(freqPairs.rbegin(), freqPairs.rend());

    // Constrói a permutação priorizando os pares mais frequentes
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

    // Adiciona ferramentas restantes que não foram usadas
    for (int i = 0; i < numTools; ++i) {
        if (!used[i]) {
            sol.permutation.push_back(i);
        }
    }

    return sol;
}




// ----------------------------------------------------------------------------
// Função de vizinhança principal: chama a operação conforme o tipo (movType)
// ----------------------------------------------------------------------------
solTIP TIP::neighbor(solTIP sol) {
    switch (movType) {
        case 1: return swapNeighbor(sol);         // Troca dois elementos
        case 2: return insertionNeighbor(sol);    // Remove de i e insere em j
        case 3: return twoOptNeighbor(sol);        // Reverte trecho entre i e j
        case 4: return randomNeighbor(sol);        // Escolhe uma das 3 acima aleatoriamente
        default: return swapNeighbor(sol);        // Padrão: swap
    }
}

// ----------------------------------------------------------------------------
// Operação 1: Vizinho por troca (swap)
// ----------------------------------------------------------------------------
solTIP TIP::swapNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    while (i == j) j = dist(rndEngine);
    std::swap(sol.permutation[i], sol.permutation[j]);
    sol.evalSol = evaluate(sol);
    return sol;
}

// ----------------------------------------------------------------------------
// Operação 2: Vizinho por inserção (insertion)
// ----------------------------------------------------------------------------
solTIP TIP::insertionNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    while (i == j) j = dist(rndEngine);

    int elem = sol.permutation[i];
    if (i < j) {
        for (int k = i; k < j; ++k)
            sol.permutation[k] = sol.permutation[k + 1];
        sol.permutation[j] = elem;
    } else {
        for (int k = i; k > j; --k)
            sol.permutation[k] = sol.permutation[k - 1];
        sol.permutation[j] = elem;
    }
    sol.evalSol = evaluate(sol);
    return sol;
}

// ----------------------------------------------------------------------------
// Operação 3: Vizinho por 2-opt (reversão de trecho [i..j])
// ----------------------------------------------------------------------------
solTIP TIP::twoOptNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(0, numTools - 1);
    int i = dist(rndEngine), j = dist(rndEngine);
    if (i > j) std::swap(i, j);
    if (i == j) {
        j = (i + 1) % numTools;
        if (i > j) std::swap(i, j);
    }
    std::reverse(sol.permutation.begin() + i, sol.permutation.begin() + j + 1);
    sol.evalSol = evaluate(sol);
    return sol;
}

// ----------------------------------------------------------------------------
// Operação 4: Vizinho aleatório (swap, insertion ou 2-opt)
// ----------------------------------------------------------------------------
solTIP TIP::randomNeighbor(solTIP sol) {
    std::uniform_int_distribution<int> dist(1, 3);  // 1=swap, 2=insertion, 3=2-opt
    int choice = dist(rndEngine);

    switch (choice) {
        case 1: return swapNeighbor(sol);
        case 2: return insertionNeighbor(sol);
        case 3: return twoOptNeighbor(sol);
        default: return sol; // fallback (não deve ocorrer)
    }
}

// ----------------------------------------------------------------------------
// Avaliação de solução: calcula custo com base no modo (1 ou 2/3)
// ----------------------------------------------------------------------------
double TIP::evaluate(solTIP sol) {
    if (mode == 1) {
        // --------------------------
        // MODO 1: sequência de operações
        // --------------------------
        std::unordered_map<int, int> pos;
        for (int slot = 0; slot < (int)sol.permutation.size(); ++slot) {
            int toolID = uniqueTools[sol.permutation[slot]];
            pos[toolID] = slot;
        }
        double cost = 0.0;
        for (size_t k = 0; k + 1 < toolSequence.size(); ++k) {
            int t1 = toolSequence[k], t2 = toolSequence[k + 1];
            int s1 = pos[t1], s2 = pos[t2];
            int delta = std::abs(s1 - s2);
            int d = std::min(delta, magazineSize - delta);
            cost += d;
        }
        return cost;
    } else {
        // --------------------------
        // MODO 2 ou 3: matriz de frequência
        // --------------------------
        std::vector<int> pos(numTools);
        for (int slot = 0; slot < (int)sol.permutation.size(); ++slot)
            pos[sol.permutation[slot]] = slot;

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

// ----------------------------------------------------------------------------
// Retorna a melhor solução registrada globalmente
// ----------------------------------------------------------------------------
solTIP TIP::getBestSol() {
    return bestSol;
}

// ----------------------------------------------------------------------------
// Converte solução para vetor de ferramentas decodificadas
// ----------------------------------------------------------------------------
std::vector<int> TIP::decodeSolution(const solTIP &sol) const {
    std::vector<int> mag(magazineSize, -1);
    for (int i = 0; i < (int)sol.permutation.size(); ++i) {
        int toolIdx = sol.permutation[i];
        int toolID = uniqueTools[toolIdx];
        mag[i] = toolID;
    }
    return mag;
}

// ----------------------------------------------------------------------------
// Define quais réplicas usarão construção gulosa
// ----------------------------------------------------------------------------
void TIP::setHeuristicReplicas(const std::unordered_set<int>& replicas) {
    heuristicReplicas = replicas;
}
