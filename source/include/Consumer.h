#ifndef __CONSUMER_H__
#define __CONSUMER_H__

#include <memory>
#include <thread>  
#include <vector> 
#include <tuple>      
#include <iostream>
#include <sstream>
#include "Pool.h"

using namespace std;

template<typename S>
class Consumer{
	private:
		unsigned threadCount;
		vector<thread> threads;
		vector<Pool> buff;
		atomic<unsigned> index{0};
		atomic<int> bestV{INT_MAX};
		atomic<int> indexPT{0};
		atomic<int> maxDif{0};
		bool finish{true};
		mutex mtxSol;
		atomic_int end{0};
		int maxEnd{0};
		S bestSol;
	public:		
		Consumer(unsigned nTh);
		void execAsync(Node* item);
		void run(unsigned i);
		void finished();
		bool theEnd();
		bool theEnd(S sol_);
		void setMaxEnd(int maxEnd_);
		void setMaxEnd();
		S getBestSol();
		int getEnd();
		atomic<int>* getIndexPT();
		atomic<int>* getMaxDif();
		int getStopC();
};



template<typename S>
Consumer<S>::Consumer(unsigned nTh){
	//armazena o numero de threads reuisitado nth
	threadCount = nTh;
	//cria um bufer para cada thread, essas são as filas de tarefa pool
	for (unsigned n = 0; n != threadCount; ++n) {
		buff.emplace_back();
	}
	//cria n threads, cada uma irá executar com o i = 0, 1, 2
	for (unsigned n = 0; n != threadCount; ++n) {
		threads.emplace_back([&, n]{ run(n); });
	}
}

template<typename S>
void Consumer<S>::run(unsigned i){
	//as tarefas node são criadas dinamicamente foda da thread e passada como ponteiro, aqui ele declara o ponteiro para uma réplica/tarefa 
	Node* c;
	//Fica esperando até que todas as tarefas tenham sido finalizadas
	while(finish){
		//em cada interação tenta pegar uma tarefa node que agora é a variável c da sua fila buff
		for(unsigned n = 0; n != threadCount; ++n){
			if((c = buff[(i + n) % threadCount].pop())){
				//aqui é onde a réplica faz mcmc
				c->run();
				break;
			}
		}
	}

}

template<typename S>
void Consumer<S>::finished(){
// End the thread	
	while(!(maxEnd <= end)) finish = true;	
	finish = false;
	for(auto& t:threads) t.join();
}

//versão sem solução
//Cada thread chama theEnd() quando termina sua execução.Incrementa o contador end.
//Retorna true se já atingiu o total de execuções esperadas (maxEnd).
template<typename S>
bool Consumer<S>::theEnd(){
	end++;
	return (maxEnd <= end);	
}

//versão com solução
//Se for passado um sol_, atualiza bestSol se for melhor.
template<typename S>
bool Consumer<S>::theEnd(S sol_){
 
	{
		unique_lock<mutex> lock{mtxSol};
		end++;
		if (sol_.evalSol<bestSol.evalSol) bestSol=sol_;
	}
return (maxEnd <= end);	
}

template<typename S>
void Consumer<S>::setMaxEnd(int maxEnd_){
	maxEnd = maxEnd_;	 //defini quantas tarefas devem ser concluidas
}

template<typename S>
void Consumer<S>::setMaxEnd(){
	++maxEnd;	//adciona uma tarefa ao contador esperado
}

template<typename S>
S Consumer<S>::getBestSol(){
 return bestSol;	
}

template<typename S>
int Consumer<S>::getEnd(){
 return end;	
}

template<typename S>
atomic<int>* Consumer<S>::getIndexPT(){
 return &indexPT;
}

template<typename S>
atomic<int>* Consumer<S>::getMaxDif(){
 return &maxDif;
}

//inserir nova tarefa node 
template<typename S>
void Consumer<S>::execAsync(Node* item){
	auto i = index++;
	//garante que insere em ordem circular e não tudo dentro de uma 
	buff[i % threadCount].push(item);
}

#endif
