#ifndef PROJETOSO_SIMULADOR_H
#define PROJETOSO_SIMULADOR_H

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "Configuracao.h"
#include "Escalonador.h"
#include "Tarefa.h"
#include "TCB.h"

// Tipos de evento que o gantt precisa desenhar, cada um com um elemento
// gráfico próprio (requisito 3 dos requisitos gerais e 2.2 do Projeto A).
enum class TipoEvento
{
    chegada,        // uma ativação da tarefa chegou ao sistema
    termino,        // uma ativação terminou de executar
    perdaPrazo,     // a ativação estourou o prazo (deadline)
    preempcao,      // a tarefa perdeu a CPU sem ter terminado
    sorteio,        // a tarefa começou a executar escolhida por SORTEIO (critério 5 do 4.3)
    cpuLigada,      // a CPU voltou a ter tarefa para executar
    cpuDesligada    // a CPU ficou sem tarefa e foi desligada (requisito 1.2)
};

[[nodiscard]] std::string nomeEvento(TipoEvento tipo);

struct Evento
{
    int instante;       // instante EXATO em que o evento acontece
    TipoEvento tipo;
    int idTarefa;       // -1 nos eventos que são só da CPU
    int cpu;            // -1 quando o evento não tem CPU associada
};

// "Foto" completa do sistema em um instante. Guardamos uma por tick para
// poder avançar e retroceder (requisito 1.5.2). Só tem valores (sem ponteiros),
// então copiar a struct copia tudo.
struct EstadoSistema
{
    int tick = 0;
    std::vector<TCB> tcbs;
    std::vector<bool> cpuLigada;
    // Quantos eventos já tinham acontecido neste instante. O log de eventos é
    // um vetor único; este número diz qual pedaço dele "já aconteceu" quando
    // estamos olhando para este tick (usado para retroceder e para descartar
    // o futuro depois de uma edição).
    std::size_t numEventos = 0;
};

// Resultado de uma tentativa de edição manual (requisitos 3.1 e 3.4): se não
// for possível, 'mensagem' explica o motivo; se der certo, pode trazer uma
// observação sobre quando a mudança passa a valer.
struct ResultadoEdicao
{
    bool ok;
    std::string mensagem;
};

// Simula um sistema multitarefa preemptivo de tempo compartilhado com várias
// CPUs e uma fila de prontos GLOBAL. O tempo passa em ticks de relógio, sem
// relação com o tempo real (requisito 1.1).
//
// Ordem das ações dentro de cada tick t (calcularPasso):
//   1. chegam as ativações cujo instante é t;
//   2. detecta quem estourou o prazo em t;
//   3. o escalonador escolhe quem ocupa as CPUs;
//   4. as tarefas são colocadas nas CPUs (CPU sem tarefa é desligada);
//   5. as tarefas executam 1 tick; o resultado vale a partir de t+1.
// Uma ativação que termina no instante t+1 com prazo t+1 CUMPRE o prazo.
class Simulador
{
    public:
        // Teto de segurança para o relógio: evita laço quase infinito quando o
        // arquivo tem períodos gigantescos (guardamos uma foto por tick).
        static constexpr int LIMITE_TICKS = 5000000;

    private:
        Configuracao config;
        std::unique_ptr<Escalonador> escalonador;
        unsigned int semente;

        // historico[i] = estado do sistema no instante i. 'cursor' é o instante
        // que está sendo mostrado; fica atrás do final quando o usuário retrocede.
        std::vector<EstadoSistema> historico;
        std::size_t cursor = 0;

        // traco[t][cpu] = id da tarefa que executou na CPU durante o tick
        // [t, t+1), ou -1 se a CPU estava desligada. É o que o gantt desenha.
        std::vector<std::vector<int>> tracoExec;
        std::vector<Evento> logEventos;
        bool estourouLimite = false;

        void calcularPasso();
        // Joga fora tudo o que vem depois do cursor (após retroceder e editar,
        // o futuro antigo não vale mais).
        void descartarFuturo();
        TCB* buscar(int id);

    public:
        // Lança std::invalid_argument se o algoritmo não existir.
        // 'semente' controla os sorteios de desempate; com a mesma semente a
        // simulação é sempre idêntica.
        Simulador(const Configuracao& configuracao, const std::vector<Tarefa>& tarefas,
                  unsigned int semente = std::random_device{}());
        ~Simulador();

        // ---- controle da execução ----
        // Avança 1 tick (calculando, ou apenas reexibindo se já foi calculado).
        // Retorna false se não há mais o que avançar (todas terminaram ou
        // estourou o LIMITE_TICKS).
        bool passo();
        // Volta 1 tick no histórico. Retorna false se já está no instante 0.
        bool voltar();
        // Execução completa, sem intervenção (requisito 1.5.b). Retorna true
        // se todas as tarefas terminaram.
        bool executarCompleto();

        // ---- consulta (para o depurador e o gantt) ----
        [[nodiscard]] bool terminou() const;
        [[nodiscard]] bool limiteExcedido() const;
        [[nodiscard]] int tickAtual() const;
        [[nodiscard]] const EstadoSistema& estadoAtual() const;
        [[nodiscard]] const Configuracao& configuracao() const;
        [[nodiscard]] unsigned int getSemente() const;
        // Todos os eventos já calculados e, destes, quantos "já aconteceram"
        // no tick atual (os primeiros numEventosVisiveis()).
        [[nodiscard]] const std::vector<Evento>& eventos() const;
        [[nodiscard]] std::size_t numEventosVisiveis() const;
        // Quem executou em cada CPU em cada tick; só as primeiras tickAtual()
        // linhas "já aconteceram".
        [[nodiscard]] const std::vector<std::vector<int>>& traco() const;
        // Total de ticks (até o tick atual) em que a CPU ficou desligada.
        [[nodiscard]] int ticksDesligada(int cpu) const;

        // ---- edição manual em qualquer passo (requisitos 1.5.2, 3.1 e 3.4) ----
        // Toda edição aplicada com sucesso descarta o "futuro" já calculado,
        // pois ele deixou de valer.
        // Campos: tempo_restante, duracao, periodo, prazo, ingresso.
        ResultadoEdicao editarTarefa(int id, const std::string& campo, int valor);
        ResultadoEdicao encerrarTarefa(int id);
        ResultadoEdicao alterarAlgoritmo(const std::string& nome);
        ResultadoEdicao alterarNumCpus(int numCpus);
};

#endif //PROJETOSO_SIMULADOR_H
