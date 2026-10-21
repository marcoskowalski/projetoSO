#ifndef PROJETOSO_TCB_H
#define PROJETOSO_TCB_H

#include <deque>

#include "EstadoTarefa.h"
#include "Tarefa.h"

// Task Control Block: guarda tudo o que a simulação precisa saber sobre uma
// tarefa (requisito 1.3): a Tarefa lida do arquivo + o seu estado atual.
// Como a tarefa é periódica, cada ativação é uma "execução" (job) nova, com
// prazo e tempo restante próprios.
//
// O TCB só tem valores (nada de ponteiros), então copiá-lo copia o estado
// completo. É isso que permite guardar uma cópia do sistema a cada tick e
// assim avançar/retroceder a simulação (requisito 1.5.2).
class TCB
{
    public:
        // O PDF (4.4) manda terminar a tarefa periódica após 10 execuções.
        static constexpr int ATIVACOES_MAXIMAS = 10;

    private:
        Tarefa tarefa;
        EstadoTarefa estado;
        int tempoRestante;      // ticks que faltam para terminar a ativação atual
        int instanteAtivacao;   // instante em que a ativação atual chegou
        int prazoAbsoluto;      // instanteAtivacao + prazo (deadline em tempo absoluto)
        int execucoes;          // quantas ativações já terminaram (para no 10)
        int cpu;                // CPU onde está executando; -1 se não está executando
        int tempoNoQuantum;     // ticks seguidos executados sem parar (para o quantum)

        // --- controle das ativações periódicas ---
        int ativacoesLiberadas;       // quantas ativações já chegaram (0..ATIVACOES_MAXIMAS)
        int ultimaLiberacao;          // instante da última ativação que chegou (-1 se nenhuma)
        int instanteProximaLiberacao; // instante em que a próxima ativação chega
        bool perdeuPrazo;             // a ativação ATUAL já estourou o prazo?

        // Ativações que chegaram enquanto a anterior ainda não tinha terminado
        // (acontece quando duracao > periodo ou o sistema está sobrecarregado).
        // Guardamos o instante de chegada de cada uma; elas entram em execução
        // na ordem, assim que a anterior terminar. Decisão: nenhuma ativação é
        // descartada, para que a tarefa execute de fato as 10 vezes do enunciado.
        std::deque<int> pendentes;

    public:
        explicit TCB(const Tarefa& tarefa);
        ~TCB();

        // Começa uma nova ativação da tarefa no instante indicado.
        void novaAtivacao(int instante);

        // A próxima ativação já deve chegar neste instante (ou antes)?
        [[nodiscard]] bool temLiberacaoEm(int instante) const;

        // Faz chegar a próxima ativação: ela vira a ativação atual se a tarefa
        // estiver inativa, ou entra na fila de pendentes se a anterior não terminou.
        // Retorna o instante nominal de chegada.
        int liberarAtivacao();

        // Chamado quando o tempo restante chegou a zero: conta a execução e
        // passa para a próxima ativação pendente, ou para inativa/terminada.
        void concluirAtivacao();

        // Encerra a tarefa à força (usado na edição manual durante a simulação).
        void encerrar();

        [[nodiscard]] const Tarefa& getTarefa() const;
        // Acesso para a edição manual de parâmetros (requisito 3.4).
        [[nodiscard]] Tarefa& tarefaMutavel();
        [[nodiscard]] EstadoTarefa getEstado() const;
        [[nodiscard]] int getTempoRestante() const;
        [[nodiscard]] int getInstanteAtivacao() const;
        [[nodiscard]] int getPrazoAbsoluto() const;
        [[nodiscard]] int getExecucoes() const;
        [[nodiscard]] int getCpu() const;
        [[nodiscard]] int getTempoNoQuantum() const;
        [[nodiscard]] int getAtivacoesLiberadas() const;
        [[nodiscard]] int getUltimaLiberacao() const;
        [[nodiscard]] int getInstanteProximaLiberacao() const;
        [[nodiscard]] bool getPerdeuPrazo() const;
        [[nodiscard]] std::size_t getNumPendentes() const;

        void setEstado(EstadoTarefa estado);
        void setTempoRestante(int tempoRestante);
        void setExecucoes(int execucoes);
        void setCpu(int cpu);
        void setTempoNoQuantum(int tempoNoQuantum);
        void setPerdeuPrazo(bool perdeu);
        void setInstanteProximaLiberacao(int instante);
};

#endif //PROJETOSO_TCB_H
