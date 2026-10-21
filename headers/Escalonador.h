#ifndef PROJETOSO_ESCALONADOR_H
#define PROJETOSO_ESCALONADOR_H

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "TCB.h"

// Resultado de uma chamada ao escalonador.
struct ResultadoEscalonamento
{
    std::vector<TCB*> escolhidas;   // até numCpus tarefas, da mais para a menos prioritária
    std::vector<int> sorteadas;     // ids escolhidos por SORTEIO (critério 5): o gráfico
                                    // de gantt deve mostrar um elemento gráfico para elas
};

// Escalonador de prioridades preemptivo. A classe base cuida de escolher as
// melhores tarefas e de aplicar os critérios de desempate (requisito 4.3).
// Para criar um algoritmo novo basta herdar e implementar prioridade() e
// nome(); a simulação não precisa mudar (requisito 4.2).
class Escalonador
{
    private:
        std::mt19937 gerador;

        // Compara a e b pelos critérios 0 a 4: prioridade do algoritmo, depois
        // (1) estava executando, (2) prazo, (3) ingresso, (4) duração.
        // Retorna <0 se a é melhor, >0 se b é melhor, 0 se empatou em tudo.
        [[nodiscard]] int comparar(const TCB& a, const TCB& b) const;

    public:
        explicit Escalonador(unsigned int semente = std::random_device{}());
        virtual ~Escalonador();

        // Escolhe quais das tarefas prontas/executando devem ocupar as CPUs.
        // 'candidatas' deve conter as tarefas PRONTA e EXECUTANDO (fila global).
        ResultadoEscalonamento escolher(const std::vector<TCB*>& candidatas, int numCpus);

        // Reinicia o gerador do sorteio. O Simulador chama isto a cada tick com
        // (semente + tick), então o sorteio daquele tick é sempre o mesmo: ao
        // retroceder e avançar de novo o gantt não muda (requisito 1.5.2).
        void semear(unsigned int semente);

        // Menor valor = mais prioritária.
        [[nodiscard]] virtual int prioridade(const TCB& tcb) const = 0;
        [[nodiscard]] virtual std::string nome() const = 0;
};

// Cria o escalonador a partir do texto do arquivo ("RM", "EDF").
// Retorna nullptr se o nome não existir.
std::unique_ptr<Escalonador> criarEscalonador(const std::string& nome);

#endif //PROJETOSO_ESCALONADOR_H
