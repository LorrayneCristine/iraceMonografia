// mainTIP.cpp
#include <cstdlib>
#include <thread>
#include <iostream>
#include <string>
#include <unistd.h>               // for sysconf()
#include "../include/ExecTime.h"
#include "../include/PT.h"        // traz PT<solTIP>
#include "TIP.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: " << argv[0]
                  << " <instancia.txt> [--MODE 1|2|3] [--MOV_TYPE 1|2|3] "
                     "[--TEMP_INIT t0] [--TEMP_FIM tF] "
                     "[--N_REPLICAS R] [--MCL mcl] [--PTL ptl] "
                     "[--TEMP_DIST d] [--TYPE_UPDATE u] [--TEMP_UPDATE tu] "
                     "[--THREAD_USED T]\n";
        return 1;
    }

    // detecta núcleos de CPU
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 1;

    // valores-padrão
    double tempIni       = 0.02;
    double tempFim       = 20.0;
    int    numReplicas   = static_cast<int>(cores) - 1;
    int    MCL           = 500;
    int    PTL           = 2200;
    int    tempDist      = 2;
    int    typeUpdate    = 2;
    int    tempUpdateAux = 3;               // divisor de MCL para calcular tempUpdate
    int    threadCount   = static_cast<int>(cores)- 1;
    int    mode          = 3;               // 1 = sequência, 2 = frequência (padrão)
    int    movType     = 1;  // 1=swap,2=insertion,3=2opt
    // nome do arquivo
    std::string filename = argv[1];

    // parse das flags a partir de argv[2]
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
            // agora TEMP_UPDATE é o divisor de MCL
            tempUpdateAux = std::stoi(argv[++i]);
        }
        else if (arg == "--THREAD_USED") {
            threadCount = std::stoi(argv[++i]);
        }
    }

    // calcula o tempUpdate real
    int tempUpdate = (tempUpdateAux > 0 ? MCL / tempUpdateAux : MCL);

    // 3) cria o problema, informando se é modo frequência (mode==2)
    TIP* problem = new TIP(filename, mode, movType);

    // 4) configura o Parallel Tempering
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
// Define quais réplicas devem usar a construção heurística (ex: réplica 0 e do meio)
std::unordered_set<int> replicasHeuristicas = {0, numReplicas / 2};
problem->setHeuristicReplicas(replicasHeuristicas);

    // 5) executa e mede tempo
    ExecTime timer;
    solTIP best = algo.start(threadCount, problem);
    double time = timer.getTimeMs(); // em segundos

    // 6) exibe resultados
    std::cout << "Custo: " <<best.evalSol *2 << "\n";

    std::cout << "Tempo: " << (time) << "\n";

   int MagSize = problem->getMagazineSize();
    int NT      = problem->getUniqueTools().size();

    auto mag = problem->decodeSolution(best);
    std::cout << "Magazine: ";
    for (int i = 0; i < MagSize; ++i) {
        if (mag[i] < 0)  std::cout << "x ";
        else             std::cout << mag[i] << " ";
    }
    std::cout << "\n";
    std::cout << "Ferramentas: " << NT << "\n";

    std::cout << "Slots: " << MagSize << "\n";

    std::cout << "Ciclo: " << best.ptl << std::endl;


    delete problem;
    return 0;
}
