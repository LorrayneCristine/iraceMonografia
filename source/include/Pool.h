#ifndef __POOL_H__
#define __POOL_H__

#include <thread>		// std::this_thread::sleep_for
#include <chrono>		// std::chrono::seconds
#include <queue>		//std::queue /// a fila principal de tarefas
#include <deque>
#include <mutex> //para proteger acesso concorrente
#include <atomic>
#include <iostream> 	
#include <optional>
#include "Node.h"

using namespace std;
//A Pool é uma fila de tarefas (Node*) com controle de concorrência. Ela é usada por cada thread do Consumer para armazenar os nós (Node) que ainda precisam ser processados.

class Pool{
	private:
		queue<Node*> data; //fila de nós para executar
		std::mutex mtx; //impede condição de corrida
		atomic_int end = 0;
		int maxEnd = 0;
	public:
		Pool(); //construtor padrao
		Pool(Pool&& obj); //construtor de movimento
		Pool& operator=(const Pool& obj); //operador de cópia
		~Pool();
		size_t size();
		void push(Node* item);
		Node* pop();
};

Pool::Pool(){}
Pool::Pool(Pool&& obj){}
Pool& Pool::operator=(const Pool& obj){
 return *this;
}
Pool::~Pool(){}
		
size_t Pool::size(){
	const std::lock_guard<std::mutex> lock(mtx);
	size_t result = data.size();
	return result;
}

//adciona um node a fila protegido por mutex pra evitar colisões
void Pool::push(Node* item){
	const std::lock_guard<std::mutex> lock(mtx);
	data.push(item);
}

//Remove o primeiro elemento da fila (FIFO):Bloqueia o mutex -> Se a fila estiver vazia, retorna NULL
//Senão:	Pega o primeiro item com data.front()-> 	Remove com data.pop()->	Retorna o item

Node* Pool::pop()
{
	const std::lock_guard<std::mutex> lock(mtx);

		if (data.size() <= 0)
		{
			return NULL;
		}
		Node* item = std::move(data.front()); data.pop();
		return item;
}


#endif
