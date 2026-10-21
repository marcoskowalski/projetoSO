#include "../headers/TCB.h"

TCB::TCB(const Tarefa& tarefa)
    : tarefa(tarefa),
      estado(EstadoTarefa::inativa),            // ainda não chegou: não é "suspensa"
      tempoRestante(tarefa.getDuracao()),
      instanteAtivacao(tarefa.getIngresso()),
      prazoAbsoluto(tarefa.getIngresso() + tarefa.getPrazo()),
      execucoes(0),
      cpu(-1),
      tempoNoQuantum(0),
      ativacoesLiberadas(0),
      ultimaLiberacao(-1),
      instanteProximaLiberacao(tarefa.getIngresso()),   // 1ª ativação = instante de ingresso
      perdeuPrazo(false)
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
    perdeuPrazo = false;
    estado = EstadoTarefa::pronta;
}

bool TCB::temLiberacaoEm(int instante) const
{
    // ">=" e não "==": se uma edição manual atrasar o relógio em relação à
    // liberação agendada, a ativação não fica "perdida" para sempre.
    return ativacoesLiberadas < ATIVACOES_MAXIMAS && instante >= instanteProximaLiberacao;
}

int TCB::liberarAtivacao()
{
    const int chegada = instanteProximaLiberacao;
    ativacoesLiberadas++;
    ultimaLiberacao = chegada;
    // A próxima chegada é sempre "última + período". Guardar o instante
    // (em vez de recalcular ingresso + k*periodo) faz com que mudar o período
    // durante a simulação valha só daqui para frente.
    instanteProximaLiberacao = chegada + tarefa.getPeriodo();

    if (estado == EstadoTarefa::inativa)
        novaAtivacao(chegada);          // nada em andamento: começa já
    else
        pendentes.push_back(chegada);   // a anterior ainda não terminou: espera na fila
    return chegada;
}

void TCB::concluirAtivacao()
{
    execucoes++;
    cpu = -1;
    tempoNoQuantum = 0;
    tempoRestante = 0;

    if (!pendentes.empty())
    {
        // Já havia outra ativação esperando: ela começa agora, mas o prazo dela
        // conta a partir de quando CHEGOU (não de quando começou a executar).
        const int chegada = pendentes.front();
        pendentes.pop_front();
        novaAtivacao(chegada);
    }
    else if (execucoes >= ATIVACOES_MAXIMAS)
        estado = EstadoTarefa::terminada;   // executou as 10 vezes: fim da tarefa
    else
        estado = EstadoTarefa::inativa;     // espera a próxima ativação
}

void TCB::encerrar()
{
    estado = EstadoTarefa::terminada;
    cpu = -1;
    tempoRestante = 0;
    tempoNoQuantum = 0;
    pendentes.clear();
    ativacoesLiberadas = ATIVACOES_MAXIMAS;   // nunca mais é ativada
}

const Tarefa& TCB::getTarefa() const { return tarefa; }
Tarefa& TCB::tarefaMutavel() { return tarefa; }
EstadoTarefa TCB::getEstado() const { return estado; }
int TCB::getTempoRestante() const { return tempoRestante; }
int TCB::getInstanteAtivacao() const { return instanteAtivacao; }
int TCB::getPrazoAbsoluto() const { return prazoAbsoluto; }
int TCB::getExecucoes() const { return execucoes; }
int TCB::getCpu() const { return cpu; }
int TCB::getTempoNoQuantum() const { return tempoNoQuantum; }
int TCB::getAtivacoesLiberadas() const { return ativacoesLiberadas; }
int TCB::getUltimaLiberacao() const { return ultimaLiberacao; }
int TCB::getInstanteProximaLiberacao() const { return instanteProximaLiberacao; }
bool TCB::getPerdeuPrazo() const { return perdeuPrazo; }
std::size_t TCB::getNumPendentes() const { return pendentes.size(); }

void TCB::setEstado(EstadoTarefa novoEstado) { estado = novoEstado; }
void TCB::setTempoRestante(int novoTempo) { tempoRestante = novoTempo; }
void TCB::setExecucoes(int novasExecucoes) { execucoes = novasExecucoes; }
void TCB::setCpu(int novaCpu) { cpu = novaCpu; }
void TCB::setTempoNoQuantum(int novoTempo) { tempoNoQuantum = novoTempo; }
void TCB::setPerdeuPrazo(bool perdeu) { perdeuPrazo = perdeu; }
void TCB::setInstanteProximaLiberacao(int instante) { instanteProximaLiberacao = instante; }
