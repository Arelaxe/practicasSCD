#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"

using namespace std ;
using namespace HM ;

const int N_CLIENTES = 4;


template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

void EsperarFueraBarberia (int numero_cliente){
	chrono::milliseconds duracion_esperar( aleatorio<20,200>() );
	cout << "El cliente " << numero_cliente << " espera fuera" << endl;
	this_thread::sleep_for( duracion_esperar );
	cout << "El cliente " << numero_cliente << " ha terminado su espera" << endl;
}

void CortarPeloACliente(){
	chrono::milliseconds duracion_cortar( aleatorio<20,200>() );
	cout << "El barbero empieza a cortarle el pelo al cliente" << endl;
	this_thread::sleep_for( duracion_cortar );
	cout << "El barbero ha terminado de cortarle el pelo" << endl;
}

class Barberia : public HoareMonitor
{
  private:
	CondVar barbero;
	CondVar sala_espera;
	CondVar cortandose;
  public:
    Barberia ();
    void cortarPelo (int numero_cliente);
	void siguienteCliente ();
	void finCliente ();
};

Barberia :: Barberia (){
	barbero = newCondVar();
	sala_espera = newCondVar();
	cortandose = newCondVar();
}

void Barberia :: cortarPelo (int numero_cliente){
	if (!cortandose.empty()){ // Si hay alguien cortándose el pelo, se espera a que esté libre
		sala_espera.wait();
		cout << "El cliente " << numero_cliente << " se va a cortar el pelo" << endl;
	}
	else if (!barbero.empty()){ // Si el barbero se está echando una siesta, se le despierta
		cout << "El cliente " << numero_cliente << " despierta al barbero para cortarse el pelo" << endl;
		barbero.signal();
	}

	cortandose.wait(); // El nuevo cliente se empieza a cortar el pelo y hay que esperarse
}

void Barberia :: siguienteCliente (){
	if (sala_espera.empty()){ // Si no hay nadie esperando, el barbero duerme
		cout << "El barbero duerme esperando al siguiente cliente para cortarse el pelo" << endl;
		barbero.wait();
	}
	else // Si hay alguien esperando le deja pasar
		sala_espera.signal();
	
}

void Barberia :: finCliente (){
	cout << "El barbero termina y le dice al cliente que se vaya fuera" << endl;
	cortandose.signal(); // El cliente ha terminado de cortarse el pelo
}

void funcion_hebra_barbero ( MRef<Barberia> monitor ){
	while ( true ){
		monitor->siguienteCliente ();
		CortarPeloACliente ();
		monitor->finCliente ();
	}
}

void funcion_hebra_cliente ( MRef<Barberia> monitor, int numero_cliente){
	while ( true ){
		monitor->cortarPelo (numero_cliente);
		EsperarFueraBarberia (numero_cliente);
	}
}

int main()
{
	MRef<Barberia> monitor = Create<Barberia>(  );

	thread barbero (funcion_hebra_barbero, monitor);
	thread clientes[N_CLIENTES];

	for (int i = 0; i < N_CLIENTES; i++){
		clientes[i] = thread(funcion_hebra_cliente, monitor, i);
	}

	barbero.join ();
	for (int i = 0; i < N_CLIENTES; i++){
		clientes[i].join();
	}
}
