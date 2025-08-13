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
#include "../include/Problem.h"  // Interface genérica para problemas do Parallel Tempering

// -----------------------------------------------------------------------------
// Estrutura solTIP: representa uma solução para o problema TIP
// -----------------------------------------------------------------------------
struct solTIP : public solution {
    std::vector<int> permutation;               // Permutação dos índices internos das ferramentas
    double evalSol = std::numeric_limits<double>::infinity();  // Avaliação (custo) da solução
    int ptl = -1;                               // Ciclo PTL no qual essa solução foi encontrada
    int Nup = 0, Ndown = 0;                     // Flags para indicar se a réplica está subindo ou descendo
};

// -----------------------------------------------------------------------------
// Classe TIP: Tool Indexing Problem
// Herda da interface Problem<solTIP> para integração com o PT
// -----------------------------------------------------------------------------
class TIP : public Problem<solTIP> {
private:
    // -------------------------------------------------------------------------
    // Dados da instância
    // -------------------------------------------------------------------------
    int numOps;                                 // Número de operações (modo 1: sequência)
    int numTools;                               // Número de ferramentas distintas
    int magazineSize;                           // Número de slots disponíveis (tamanho do magazine)
    int mode;                                   // Tipo de entrada: 1 = sequência, 2 = frequência + aleatória, 3 = frequência + heurística
    bool useFreq;                               // True se deve usar matriz de frequência em vez de sequência

    std::vector<int> toolSequence;              // Sequência de ferramentas (modo 1)
    std::vector<int> uniqueTools;               // Conjunto de ferramentas únicas (ordenadas)
    std::unordered_map<int, int> toolToIndex;   // Mapeia ferramenta original → índice interno

    std::vector<std::vector<int>> frequencyMatrix;  // Matriz de frequência entre pares de ferramentas
    std::vector<std::vector<int>> distanceMatrix;   // Matriz de distância circular (modo 1)

    // -------------------------------------------------------------------------
    // Estratégia de construção
    // -------------------------------------------------------------------------
    std::unordered_set<int> heuristicReplicas;  // IDs das réplicas que usarão construção gulosa

    // -------------------------------------------------------------------------
    // Configuração de vizinhança (movimentos locais)
    // -------------------------------------------------------------------------
    int movType;                                // Tipo de movimento: 1=swap, 2=insertion, 3=2-opt, 4=random

    solTIP swapNeighbor(solTIP sol);            // Movimento de troca (swap)
    solTIP insertionNeighbor(solTIP sol);       // Movimento de inserção
    solTIP twoOptNeighbor(solTIP sol);          // Movimento 2-opt
    solTIP randomNeighbor(solTIP sol);          // Movimento aleatório

    // -------------------------------------------------------------------------
    // Controle paralelo
    // -------------------------------------------------------------------------
    std::atomic<int> endCounter{0};             // Contador de finalizações (usado no critério de parada)
    int maxEnd = 0;                             // Quantidade máxima de réplicas que podem finalizar

    solTIP bestSol;                             // Melhor solução global encontrada até o momento
    std::mutex bestSolMutex;                    // Mutex para acesso seguro à melhor solução

    // -------------------------------------------------------------------------
    // Aleatoriedade
    // -------------------------------------------------------------------------
    static thread_local std::mt19937 rndEngine; // Gerador aleatório específico para cada thread

public:
    // -------------------------------------------------------------------------
    // Construtor e destrutor
    // -------------------------------------------------------------------------
    TIP(const std::string &filename,
        int mode     = 1,
        int movType  = 1);                      // Inicializa o problema com o arquivo e configurações

    ~TIP();                                     // Destrutor padrão

    // -------------------------------------------------------------------------
    // Implementação da interface Problem<solTIP>
    // -------------------------------------------------------------------------
    solTIP construction() override;             // Construção padrão (sem distinção de réplica)
    solTIP construction(int replicaID);         // Construção com lógica específica por réplica
    solTIP neighbor(solTIP sol) override;       // Geração de vizinho a partir da solução atual
    double evaluate(solTIP sol) override;       // Avaliação do custo de uma solução

    // -------------------------------------------------------------------------
    // Métodos auxiliares
    // -------------------------------------------------------------------------
    void setHeuristicReplicas(const std::unordered_set<int>& replicas);  // Define quais réplicas usam heurística
    std::vector<int> decodeSolution(const solTIP &sol) const;            // Decodifica permutação em layout de magazine
    int getMagazineSize() const noexcept { return magazineSize; }        // Retorna o tamanho do magazine
    const std::vector<int>& getUniqueTools() const noexcept { return uniqueTools; } // Retorna ferramentas únicas

    void buildDistanceMatrix();                  // Calcula a matriz de distância circular (modo sequência)
    solTIP getBestSol();                         // Retorna a melhor solução global
    solTIP greedyConstructionFreq();             // Construção gulosa com base na matriz de frequência

    // -------------------------------------------------------------------------
    // Impressão (caso deseje adicionar futuramente)
    // -------------------------------------------------------------------------
};

#endif // TIP_H
