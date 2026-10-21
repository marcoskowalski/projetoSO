#ifndef PROJETOSO_TCB_H
#define PROJETOSO_TCB_H

#include "EstadoTarefa.h"
#include "Tarefa.h"

// Task Control Block: guarda tudo o que a simulação precisa saber sobre uma
// tarefa (requisito 1.3): a Tarefa lida do arquivo + o seu estado atual.
// Como a tarefa é periódica, cada ativação é uma "execução" (job) nova, com
// prazo e tempo restante próprios.
class TCB
{
    private:
        Tarefa tarefa;
        EstadoTarefa estado;
        int tempoRestante;      // ticks que faltam para terminar a ativação atual
        int instanteAtivacao;   // instante em que a ativação atual chegou
        int prazoAbsoluto;      // instanteAtivacao + prazo (deadline em tempo absoluto)
        int execucoes;          // quantas ativações já terminaram (para no 10)
        int cpu;                // CPU onde está executando; -1 se não está executando
        int tempoNoQuantum;     // ticks seguidos executados sem parar (para o quantum)

    public:
        explicit TCB(const Tarefa& tarefa);
        ~TCB();

        // Começa uma nova ativação da tarefa no instante indicado.
        void novaAtivacao(int instante);

        [[nodiscard]] const Tarefa& getTarefa() const;
        [[nodiscard]] EstadoTarefa getEstado() const;
        [[nodiscard]] int getTempoRestante() const;
        [[nodiscard]] int getInstanteAtivacao() const;
        [[nodiscard]] int getPrazoAbsoluto() const;
        [[nodiscard]] int getExecucoes() const;
        [[nodiscard]] int getCpu() const;
        [[nodiscard]] int getTempoNoQuantum() const;

        void setEstado(EstadoTarefa estado);
        void setTempoRestante(int tempoRestante);
        void setExecucoes(int execucoes);
        void setCpu(int cpu);
        void setTempoNoQuantum(int tempoNoQuantum);
};

#endif //PROJETOSO_TCB_H
