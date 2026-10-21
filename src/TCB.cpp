#include "../headers/TCB.h"
//bah
TCB::TCB(const Tarefa& tarefa)
    : tarefa(tarefa),
      estado(EstadoTarefa::suspensa),
      tempoRestante(tarefa.getDuracao()),
      instanteAtivacao(tarefa.getIngresso()),
      prazoAbsoluto(tarefa.getIngresso() + tarefa.getPrazo()),
      execucoes(0),
      cpu(-1),
      tempoNoQuantum(0)
{
}

TCB::~TCB() = default;

void TCB::novaAtivacao(int instante)
{
    instanteAtivacao = instante;
    prazoAbsoluto = instante + tarefa.getPrazo();   // o prazo conta a partir da ativação
    tempoRestante = tarefa.getDuracao();
    tempoNoQuantum = 0;
    cpu = -1;
    estado = EstadoTarefa::pronta;
}

const Tarefa& TCB::getTarefa() const { return tarefa; }
EstadoTarefa TCB::getEstado() const { return estado; }
int TCB::getTempoRestante() const { return tempoRestante; }
int TCB::getInstanteAtivacao() const { return instanteAtivacao; }
int TCB::getPrazoAbsoluto() const { return prazoAbsoluto; }
int TCB::getExecucoes() const { return execucoes; }
int TCB::getCpu() const { return cpu; }
int TCB::getTempoNoQuantum() const { return tempoNoQuantum; }

void TCB::setEstado(EstadoTarefa novoEstado) { estado = novoEstado; }
void TCB::setTempoRestante(int novoTempo) { tempoRestante = novoTempo; }
void TCB::setExecucoes(int novasExecucoes) { execucoes = novasExecucoes; }
void TCB::setCpu(int novaCpu) { cpu = novaCpu; }
void TCB::setTempoNoQuantum(int novoTempo) { tempoNoQuantum = novoTempo; }
