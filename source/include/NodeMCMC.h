#ifndef __NODEMCMC_H__
#define __NODEMCMC_H__

#include <random>
#include <atomic>
#include <mutex>
#include <sstream>
#include "Consumer.h"
static std::mutex node_print_mutex;


template<typename S>
class NodeMCMC: public Node{
	private:
		int MCL; //numero de interações MCMC
		atomic_int nUp = 0; //contadores de fluxo
		atomic_int nDown = 0; //contadores de fluxo
		int currentMCMC = 0;//contador de execução 
		int evalValue = 0;// Armazena valor da energia
		double temp;// Temperatura da réplica
		Problem<S>* prob;// Ponteiro para problema (eval, vizinho etc.)
		S sol;// Solução atual, vizinha e auxiliar
		S neigh;
		S neighAux;
		int accept = 0; // Quantos vizinhos foram aceitos
		S bestSol;// Melhor solução encontrada até agora
		Consumer<S>* pool;// Ponteiro para o Consumer (distribuidor de tarefas)

		bool firstTemp = false;// Flags para round-trip
		bool lastTemp = false;// Flags para round-trip
		atomic_bool fase = true;
		atomic_bool final = false;
		atomic_bool stop = false;
		atomic_bool th2 = false;
		std::function<void(void)> exec;// função executável (não usada aqui)
		std::default_random_engine gen;// motor aleatório
		std::uniform_real_distribution<double> dis;// distribuição uniforme [0,1]

		mutex mtxPara;// mutex auxiliar



	public:
		
		NodeMCMC(int MCL_, atomic<int>* PTL_, double temp_, Problem<S>* prob_, Consumer<S>* pool_, int replicaID);
		~NodeMCMC();
		void run();
		bool ready();
		bool notify();
		bool observer(Node* from);
		double getTemp();
		int getEnergia();
		double getAccept();
		void start();
		void setSolUp();
		double getFlow();
		void resetFlow();
		void threadRun();
		S getSol();
		void updateFlow();
		void setLastTemp();
		void setFirstTemp();
		void trySetLabels();
		void setTemp(double t);
		void setSol(S sol_);
		bool setBestSol(S sol_);
};

//Inicializa os atributos.		Salva o número de passos MCMC (MCL), temperatura, ponteiros de problema e thread-pool.
template<typename S>
NodeMCMC<S>::NodeMCMC(int MCL_, atomic<int>* PTL_, double temp_, Problem<S>* prob_, Consumer<S>* pool_, int replicaID)
:MCL(MCL_)
,temp(temp_)
,prob(prob_)
,pool(pool_)
{
	execMax = PTL_;// ponteiro para o contador de execuções globais
	indexPT = pool_->getIndexPT();// id compartilhado
	sol = prob->construction(replicaID); // ✅ agora sim, passando o ID correto da réplica
	bestSol = sol;  // Melhor solução começa como a inicial
				
}

template<typename S>
NodeMCMC<S>::~NodeMCMC(){
}

template<typename S>
void NodeMCMC<S>::run(){
	
	// calc MCMC
	for (int i = 0; i < MCL; i++){
		
		// create neighbor									
		neigh = prob->neighbor(sol);

		// Calc evaluate function 
		neigh.evalSol = prob->evaluate(neigh); 
		
		// ⬅️ Adicione esta linha logo aqui
		neigh.ptl = execAtual;

		// Save best sol			
		setBestSol(neigh);
				
		//calc delta
		double delta = neigh.evalSol - sol.evalSol;
					
		// Less or equal to 0, accepted
		if (delta <= 0.0){
			// 
			sol = neigh;
			++accept;
		}else{
			//Calc of probability of acceptance
			double param, probab;
			param = -(delta/temp);
			probab = exp(param);
					
			//Swap accept
			if(dis(gen) <= probab){
				sol = neigh;
				++accept;
			} //End if					
		} //End if/else 
	
	} //End for

	if(theEnd()){
		pool->theEnd(bestSol);
		endN = true;
	}
	
	// Reset to receve new request
	reset();
	// Notify nodes forward	
	notify();
}

// Checks dependencies against connected nodes
template<typename S>
bool NodeMCMC<S>::ready(){
	
	for(vector<std::pair<Node*,bool>>::iterator it = edgeto.begin(); it != edgeto.end(); it++){
		if(!it->second) return false;
	}
	
	return true;
}

// Notifies all forward nodes connected to the MC node
template<typename S>
bool NodeMCMC<S>::notify(){
	
	for(vector<std::pair<Node*,bool>>::iterator it = edgeFrom.begin(); it != edgeFrom.end(); it++){
		Node* to = it->first;
		if(!to->observer(this))return false; 	
	}
	
	return true;
}

// Received notifications from connected nodes
template<typename S>
bool NodeMCMC<S>::observer(Node* from){
	bool status = false;
	bool ready = true;
	
	mtxNode.lock();
	for(vector<std::pair<Node*,bool>>::iterator it = edgeto.begin(); it != edgeto.end(); it++){
		
		if(it->first == from){
			it->second = true;
			status = true;
		}
		
		if(!it->second) ready = false;
	}
	mtxNode.unlock();
	
	// Check dependency, if not, place it in the execution queue
	if((ready) && (!finish() || !endN)){
		pool->execAsync(this);	
	}
	
	return status;
}

// Returns the temperature
template<typename S>
double NodeMCMC<S>::getTemp(){
	return temp;
}

// Return the energy
template<typename S>
int NodeMCMC<S>::getEnergia(){
	return sol.evalSol;
}

// Return current solution
template<typename S>
S NodeMCMC<S>::getSol(){
	return sol;
}

// Change current solution
template<typename S>
void NodeMCMC<S>::setSol(S sol_){
	sol = sol_;
	setBestSol(sol);
}

// keep the best solution
template<typename S>
bool NodeMCMC<S>::setBestSol(S sol_){
	if (sol_.evalSol<bestSol.evalSol) {
		bestSol=sol_;
		return true;
	}
	
	return false;
}

// Places the node in the execution queue
template<typename S>
void NodeMCMC<S>::start(){
	pool->execAsync(this);
}

// Seta uma nova Temperatura
template<typename S>
void NodeMCMC<S>::setTemp(double t){
	temp = t;
}

// Returns the acceptance rate
template<typename S>
double NodeMCMC<S>::getAccept(){
	double rate = ((double)accept/(double)(execAtual*MCL));
	accept = 0;
	return rate;
}

// Check if it is the last temperature
template<typename S>
void NodeMCMC<S>::setLastTemp(){
	lastTemp = true;
}

// Check if it is the first temperature
template<typename S>
void NodeMCMC<S>::setFirstTemp(){
	firstTemp = true;
}

// Load roundtrip labels
template<typename S>
void NodeMCMC<S>::trySetLabels(){
	if(firstTemp){
		sol.Nup = true;
		sol.Ndown = false;
	} else if(lastTemp){
		sol.Nup = false;
		sol.Ndown = true;
	}
}

// Update the flow 
template<typename S>
void NodeMCMC<S>::updateFlow(){
	if(sol.Nup) ++nUp;
	else if(sol.Ndown) ++nDown;
}

// Return the flow
template<typename S>
double NodeMCMC<S>::getFlow(){	
	
	if((nUp+nDown) == 0) return 0;
	double f = ((double)nUp/(double)(nUp+nDown));
	return f; 
}

// Reset the flow
template<typename S>
void NodeMCMC<S>::resetFlow(){	
		
	nUp = 0;
	nDown = 0;
}

#endif
