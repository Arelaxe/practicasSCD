#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include "HoareMonitor.h"
#include <chrono>

using namespace std;
using namespace HM;

const int TAM = 3;


//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

int producir( )
{
	// calcular milisegundos aleatorios de duración de la acción de producir)
   chrono::milliseconds duracion_producir( aleatorio<20,200>() );

	int producto = aleatorio<0,2>();

	cout << "Estanquero " << "  :"
          << " empieza a producir ingrediente " << producto << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_producir );

   // informa de que ha terminado de fumar

    cout << "Estanquero termina de producir, comienza espera de consumición." << endl;

	return producto;
}

void fumar( int num_fumador )
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar

    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;

   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar

    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;

}

class Estanco : public HoareMonitor
{
  private:
	bool disponibles[TAM] = {false};
    CondVar ing_disp[TAM];
	CondVar mostrador_libre;
  public:
    Estanco ();
    void obtenerIngrediente (int i);
	void ponerIngrediente (int i);
	void esperarRecogidaIngrediente ();
};

Estanco :: Estanco (){
	for (int i = 0; i < TAM; i++)
		ing_disp[i] = newCondVar();

	mostrador_libre = newCondVar();
}

void Estanco :: obtenerIngrediente (int i)
{
	if(!disponibles[i])      // Si el ingrediente no está disponible, hay que esperar
		ing_disp[i].wait();

	cout << "Retirado ingrediente " << i << endl; // Se retira el ingrediente

	mostrador_libre.signal(); // El mostrador queda libre y se avisa de ello
}

void Estanco :: ponerIngrediente (int i){

	cout << "Puesto ingr.: " << i << endl; // Se pone el ingrediente en el mostrador y se avisa de que está disponible
	ing_disp[i].signal();
}

void Estanco :: esperarRecogidaIngrediente (){
	bool libre = true;
	for (int i = 0; i < TAM && libre; i++){
		if (disponibles[i])
			libre = false;
	}
	if (!libre) // Si el mostrador no está libre hay que esperar
		mostrador_libre.wait();
}

void  funcion_hebra_fumador( MRef<Estanco> monitor, int num_fumador )
{
   while( true )
   {
			monitor->obtenerIngrediente (num_fumador);
			fumar (num_fumador);
   }
}

void funcion_hebra_estanquero( MRef<Estanco> monitor )
{
	int num_producto;

	while ( true ){
		num_producto = producir ();
		monitor->ponerIngrediente (num_producto);
		monitor->esperarRecogidaIngrediente ();
	}
}


int main()
{
   MRef<Estanco> monitor = Create<Estanco>(  );

   thread hebra_estanquero (funcion_hebra_estanquero, monitor), 
          hebra_fumador_0 (funcion_hebra_fumador, monitor, 0), 
          hebra_fumador_1 (funcion_hebra_fumador, monitor, 1),                
          hebra_fumador_2 (funcion_hebra_fumador, monitor, 2);

	hebra_estanquero.join ();
	hebra_fumador_0.join ();
	hebra_fumador_1.join ();
	hebra_fumador_2.join ();
}
