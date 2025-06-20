#ifndef TIP_H
#define TIP_H

#include <string>
#include <vector>
#include <fstream>
#include <random>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include "../include/Problem.h"    // declara Problem<solTIP>

// ------------------------------------------------------------------
// solTIP: estrutura que representa uma solução do TIP
// ------------------------------------------------------------------
struct solTIP : public solution {
    std::vector<int> permutation;  // permutação das ferramentas (índices internos)
    double evalSol = std::numeric_limits<double>::infinity();
    int Nup = 0, Ndown = 0;        // contadores auxiliares, se necessário
};

// ------------------------------------------------------------------
// TIP: Tool Indexing Problem, herda de Problem<solTIP>
// ------------------------------------------------------------------
class TIP : public Problem<solTIP> {
private:
    int numOps;                   // número de operações (modo sequência)
    int numTools;                 // número de ferramentas distintas
    int magazineSize;             // tamanho do magazine (slots)

    bool useFreq;                 // se true, lê matriz de frequência em vez de sequência
    std::vector<std::vector<int>> frequencyMatrix;

    std::vector<int> toolSequence;                // sequência de operações (modo 1)
    std::vector<int> uniqueTools;                 // ferramentas distintas (ordenadas)
    std::unordered_map<int, int> toolToIndex;     // mapeamento ferramenta → índice interno

    std::vector<std::vector<int>> distanceMatrix; // matriz de distâncias circulares (modo 1)

    int movType;                 // 1=swap, 2=insertion, 3=two-opt

    std::atomic<int> endCounter{0};  // contador para theEnd()
    int maxEnd = 0;                  // limite de chamadas a theEnd()

    solTIP bestSol;                  // melhor solução global
    std::mutex bestSolMutex;         // protege acesso a bestSol

    // gerador de números aleatórios, um por thread
    static thread_local std::mt19937 rndEngine;

    // implementações de vizinhança
    solTIP swapNeighbor(solTIP sol);
    solTIP insertionNeighbor(solTIP sol);
    solTIP twoOptNeighbor(solTIP sol);

public:
    // Construtor:
    //   useFreqMode = true → modo frequência
    //   movType     = 1 (swap), 2 (insertion), 3 (two-opt)
    TIP(const std::string &filename,
        bool useFreqMode = false,
        int movType      = 1);
    ~TIP();

    // Interface Problem<solTIP>
    solTIP construction() override;
    solTIP neighbor(solTIP sol) override;
    double evaluate(solTIP sol) override;

    // Decodifica solução em layout de magazine (tamanho magazineSize)
    std::vector<int> decodeSolution(const solTIP &sol) const;
    int getMagazineSize() const noexcept { return magazineSize; }
    const std::vector<int>& getUniqueTools() const noexcept { return uniqueTools; }

    // Auxiliares de execução
    void buildDistanceMatrix();
    solTIP getBestSol();

    // Impressão
    void printCircularDistanceMatrix(const solTIP &sol) const;
    void printDistanceMatrix() const;
};

#endif // TIP_H
