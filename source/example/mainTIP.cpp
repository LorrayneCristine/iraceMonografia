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
                  << " <instancia.txt> [--MODE 1|2] [--TEMP_INIT t0] [--TEMP_FIM tF] "
                     "[--N_REPLICAS R] [--MCL mcl] [--PTL ptl] "
                     "[--TEMP_DIST d] [--TYPE_UPDATE u] [--TEMP_UPDATE div] "
                     "[--THREAD_USED T]\n";
        return 1;
    }

    // detecta núcleos de CPU
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores <= 0) cores = 1;

    // valores-padrão
    double tempIni       = 0.02;
    double tempFim       = 20.0;
    int    numReplicas   = 7;
    int    MCL           = 500;
    int    PTL           = 2000;
    int    tempDist      = 2;
    int    typeUpdate    = 2;
    int    tempUpdateAux = 3;               // divisor de MCL para calcular tempUpdate
    int    threadCount   = 7;
    int    mode          = 2;               // 1 = sequência, 2 = frequência (padrão)

    // nome do arquivo
    std::string filename = argv[1];

    // parse das flags a partir de argv[2]
    for (int i = 2; i + 1 < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--MODE") {
            mode = std::stoi(argv[++i]);
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
    TIP* problem = new TIP(filename, mode == 2);

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

    // 5) executa e mede tempo
    ExecTime timer;
    solTIP best = algo.start(threadCount, problem);

    // 6) exibe resultadoss
    std::cout << best.evalSol *2;

    delete problem;
    return 0;
}
