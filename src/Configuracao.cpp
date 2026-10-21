//
// Created by mvmko on 20/10/2026.
//

#include "Configuracao.h"

Configuracao::Configuracao(int quantum, int numCpus, std::string algoritmo)
    : quantum(quantum), numCpus(numCpus), algoritmo(algoritmo){
}
Configuracao::Configuracao() : quantum(0), numCpus(0), algoritmo(""){}
Configuracao::~Configuracao() = default;

std::string Configuracao::getAlgoritmo() const{
    return algoritmo;
}
int Configuracao::getNumCpus() const{
    return numCpus;
}
int Configuracao::getQuantum() const{
    return quantum;
}

void Configuracao::setAlgoritmo(std::string algoritmo){
    this->algoritmo = algoritmo;
}
void Configuracao::setNumCpus(int numCpus){
    this->numCpus = numCpus;
}
void Configuracao::setQuantum(int quantum){
    this->quantum = quantum;
}



