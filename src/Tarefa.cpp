//
// Created by mvmko on 20/10/2026.
//

#include "../headers/Tarefa.h"

Tarefa::Tarefa() = default;
Tarefa::Tarefa(int id, int periodo, int ingresso,
                int duracao, int prazo, int cor)
    : id(id),
    periodo(periodo),
    ingresso(ingresso),
    duracao(duracao),
    prazo(prazo),
    cor(cor){
}
Tarefa::~Tarefa() = default;

unsigned int Tarefa::getCor() const{
    return cor;
}
int Tarefa::getDuracao() const{
    return duracao;
}
int Tarefa::getId() const{
    return id;
}
int Tarefa::getIngresso() const{
    return ingresso;
}
int Tarefa::getPeriodo() const{
    return periodo;
}
int Tarefa::getPrazo() const{
    return prazo;
}

void Tarefa::setCor(unsigned int cor){
    this->cor = cor;
}
void Tarefa::setDuracao(int duracao){
    this->duracao = duracao;
}
void Tarefa::setIngresso(int ingresso){
    this->ingresso = ingresso;
}
void Tarefa::setPeriodo(int periodo){
    this->periodo = periodo;
}
void Tarefa::setPrazo(int prazo){
    this->prazo = prazo;
}






