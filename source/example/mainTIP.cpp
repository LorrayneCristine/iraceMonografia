// mainTIP.cpp
#include <cstdlib>
#include <thread>
#include <iostream>
#include <string>
#include <unistd.h>               // Para sysconf(): detectar núcleos da CPU
#include "../include/ExecTime.h" // Utilitário para medir tempo de execução
#include "../include/PT.h"       // Traz a classe Parallel Tempering (PT<solTIP>)
#include "TIP.h"                 // Define o problema TIP

int main(int argc, char* argv[]) {
    // ------------------------------------------------------------
    // Verificação mínima de argumentos
    // ------------------------------------------------------------
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " <instancia.txt> [--MODE 1|2|3] [--MOV_TYPE 1|2|3|4] "
                     "[--TEMP_INIT t0] [--TEMP_FIM tF] "
                     "[--N_REPLICAS R] [--MCL mcl] [--PTL ptl] "
                     "[--TEMP_DIST d] [--TYPE_UPDATE u] [--TEMP_UPDATE tu] "
                     "[--THREAD_USED T]\n";
        return 1;
    }

    // ------------------------------------------------------------
    // Detecta número de núcleos disponíveis
    // ------------------------------------------------------------
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 1; // fallback para 1 thread

    // ------------------------------------------------------------
    // Valores-padrão dos parâmetros (com base no preset informado)
    // ------------------------------------------------------------
    double tempIni       = 0.01;                          // Temperatura inicial
    double tempFim       = 20.0;                          // Temperatura final
    int    numReplicas   = static_cast<int>(cores) - 1;   // Nº de réplicas = núcleos - 1
    int    MCL           = 500;                           // Máx ciclos por réplica (Markov Chain Length)
    int    PTL           = 2500;                          // Ciclos totais do Parallel Tempering
    int    tempDist      = 2;                             // Espaçamento das temperaturas
    int    typeUpdate    = 2;                             // Estratégia de atualização de temperatura
    int    tempUpdateAux = 4;                             // Divisor de MCL para definir freq de troca
    int    threadCount   = static_cast<int>(cores) - 1;   // Nº de threads = núcleos - 1
    int    mode          = 3;                             // Modo de leitura: 3 = matriz + heurística
    int    movType       = 4;                             // Tipo de movimento: 4 = random

    // ------------------------------------------------------------
    // Nome da instância fornecida
    // ------------------------------------------------------------
    std::string filename = argv[1];

    // ------------------------------------------------------------
    // Leitura dos argumentos da linha de comando
    // ------------------------------------------------------------
    for (int i = 2; i + 1 < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--MODE") {
            mode = std::stoi(argv[++i]);
        }
        else if (arg == "--MOV_TYPE") {
            movType = std::stoi(argv[++i]);
        }
        else if (arg == "--TEMP_INIT") {
            tempIni = std::stod(argv[++i]);
        }
        else if (arg == "--TEMP_FIM") {
            tempFim = std::stod(argv[++i]);
        }
        else if (arg == "--N_REPLICAS") {
            numReplicas = std::stoi(argv[++i]);
        }
        else if (arg == "--MCL") {
            MCL = std::stoi(argv[++i]);
        }
        else if (arg == "--PTL") {
            PTL = std::stoi(argv[++i]);
        }
        else if (arg == "--TEMP_DIST") {
            tempDist = std::stoi(argv[++i]);
        }
        else if (arg == "--TYPE_UPDATE") {
            typeUpdate = std::stoi(argv[++i]);
        }
        else if (arg == "--TEMP_UPDATE") {
            tempUpdateAux = std::stoi(argv[++i]); // divisor de MCL
        }
        else if (arg == "--THREAD_USED") {
            threadCount = std::stoi(argv[++i]);
        }
    }

    // ------------------------------------------------------------
    // Calcula quantas iterações entre cada tentativa de troca de temperatura
    // ------------------------------------------------------------
    int tempUpdate = (tempUpdateAux > 0 ? MCL / tempUpdateAux : MCL);

    // ------------------------------------------------------------
    // Cria o problema com os parâmetros informados
    // ------------------------------------------------------------
    TIP* problem = new TIP(filename, mode, movType);

    // ------------------------------------------------------------
    // Inicializa o algoritmo Parallel Tempering com as configurações
    // ------------------------------------------------------------
    PT<solTIP> algo(
        tempIni,
        tempFim,
        numReplicas,
        MCL,
        PTL,
        tempDist,
        typeUpdate,
        tempUpdate
    );

    // ------------------------------------------------------------
    // Define quais réplicas vão usar construção gulosa
    // ------------------------------------------------------------
    std::unordered_set<int> replicasHeuristicas;
    if (mode == 2) {
        // Modo 2 = 100% aleatório
        replicasHeuristicas.clear();
    } else if (mode == 3) {
        // Modo 3 = híbrido: primeira e do meio são heurísticas
        replicasHeuristicas = {0, numReplicas / 2};
    }

    // Informa ao problema quais réplicas usarão heurística gulosa
    problem->setHeuristicReplicas(replicasHeuristicas);

    // ------------------------------------------------------------
    // Executa o algoritmo e mede o tempo de execução
    // ------------------------------------------------------------
    ExecTime timer;
    solTIP best = algo.start(threadCount, problem);
    double time = timer.getTimeMs(); // Tempo total em milissegundos

    // ------------------------------------------------------------
    // Impressão dos resultados
    // ------------------------------------------------------------
    std::cout << "Custo: " << best.evalSol * 2 << "\n"; // custo final (ajustado)
    std::cout << "Tempo: " << time << "\n";             // tempo em ms

    int MagSize = problem->getMagazineSize();           // slots do magazine
    int NT      = problem->getUniqueTools().size();     // nº de ferramentas

    // Mostra o layout final do magazine
    auto mag = problem->decodeSolution(best);
    std::cout << "Magazine: ";
    for (int i = 0; i < MagSize; ++i) {
        if (mag[i] < 0) std::cout << "x ";
        else            std::cout << mag[i] << " ";
    }
    std::cout << "\n";

    // Ferramentas e slots
    std::cout << "Ferramentas: " << NT << "\n";
    std::cout << "Slots: " << MagSize << "\n";

    // Ciclo em que a melhor solução foi encontrada
    std::cout << "Ciclo: " << best.ptl << std::endl;

    // ------------------------------------------------------------
    // Libera memória e finaliza
    // ------------------------------------------------------------
    delete problem;
    return 0;
}
