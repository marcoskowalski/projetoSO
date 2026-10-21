#include "../headers/Simulador.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <stdexcept>

namespace
{
    std::string minusculas(std::string s)
    {
        for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    ResultadoEdicao erro(const std::string& mensagem) { return {false, mensagem}; }
    ResultadoEdicao sucesso(const std::string& mensagem = "") { return {true, mensagem}; }

    bool estaAtiva(const TCB& t)
    {
        return t.getEstado() == EstadoTarefa::pronta || t.getEstado() == EstadoTarefa::executando;
    }

    const char* nomeEstado(EstadoTarefa e)
    {
        switch (e)
        {
            case EstadoTarefa::inativa: return "inativa";
            case EstadoTarefa::pronta: return "pronta";
            case EstadoTarefa::executando: return "executando";
            case EstadoTarefa::suspensa: return "suspensa";
            case EstadoTarefa::terminada: return "terminada";
        }
        return "?";
    }
}

std::string nomeEvento(TipoEvento tipo)
{
    switch (tipo)
    {
        case TipoEvento::chegada: return "chegada";
        case TipoEvento::termino: return "termino";
        case TipoEvento::perdaPrazo: return "perda de prazo";
        case TipoEvento::preempcao: return "preempcao";
        case TipoEvento::sorteio: return "sorteio";
        case TipoEvento::cpuLigada: return "CPU ligada";
        case TipoEvento::cpuDesligada: return "CPU desligada";
    }
    return "?";
}

// ---------------------------------------------------------------------------
Simulador::Simulador(const Configuracao& configuracao, const std::vector<Tarefa>& tarefas,
                     unsigned int semente)
    : config(configuracao), semente(semente)
{
    escalonador = criarEscalonador(config.getAlgoritmo());
    if (!escalonador)
        throw std::invalid_argument("Algoritmo de escalonamento desconhecido: '" +
                                    config.getAlgoritmo() + "' (use RM ou EDF).");
    if (config.getNumCpus() < 1)
        throw std::invalid_argument("A quantidade de CPUs deve ser pelo menos 1.");

    // Instante 0: ninguém chegou ainda, todas as CPUs começam ligadas (as que
    // não tiverem o que fazer são desligadas e isso aparece como evento).
    EstadoSistema inicial;
    inicial.tick = 0;
    for (const Tarefa& t : tarefas) inicial.tcbs.emplace_back(t);
    inicial.cpuLigada.assign(static_cast<std::size_t>(config.getNumCpus()), true);
    inicial.numEventos = 0;
    historico.push_back(std::move(inicial));
}

Simulador::~Simulador() = default;

// ---------------------------------------------------------------------------
void Simulador::calcularPasso()
{
    EstadoSistema novo = historico[cursor];      // parte de uma cópia do estado atual
    const int t = novo.tick;
    const int numCpus = config.getNumCpus();

    // 1) Chegada das ativações no instante t.
    for (TCB& tcb : novo.tcbs)
    {
        if (!tcb.temLiberacaoEm(t)) continue;
        tcb.liberarAtivacao();
        logEventos.push_back({t, TipoEvento::chegada, tcb.getTarefa().getId(), -1});
    }

    // 2) Perda de prazo: a ativação atual ainda não terminou e o instante do
    //    prazo absoluto chegou. A tarefa continua executando (PDF, campo
    //    'prazo'); só marcamos e registramos o evento UMA vez por ativação.
    //    O evento é gravado no instante exato do prazo, mesmo que só o
    //    percebamos depois (ativação que esperou na fila de pendentes).
    for (TCB& tcb : novo.tcbs)
    {
        if (estaAtiva(tcb) && !tcb.getPerdeuPrazo() && t >= tcb.getPrazoAbsoluto())
        {
            tcb.setPerdeuPrazo(true);
            logEventos.push_back({tcb.getPrazoAbsoluto(), TipoEvento::perdaPrazo,
                                  tcb.getTarefa().getId(), tcb.getCpu()});
        }
    }

    // 3) O escalonador escolhe, entre prontas e executando (fila global), as
    //    tarefas que devem ocupar as CPUs neste tick.
    std::vector<TCB*> candidatas;
    for (TCB& tcb : novo.tcbs)
        if (estaAtiva(tcb)) candidatas.push_back(&tcb);

    // Semente = base + tick: o sorteio deste tick é sempre o mesmo, mesmo que
    // o usuário volte no tempo e avance de novo.
    escalonador->semear(semente + static_cast<unsigned int>(t));
    ResultadoEscalonamento res = escalonador->escolher(candidatas, numCpus);

    std::set<const TCB*> escolhidas(res.escolhidas.begin(), res.escolhidas.end());
    std::set<int> sorteadas(res.sorteadas.begin(), res.sorteadas.end());

    // 4) Coloca as tarefas nas CPUs.
    //    4a) Quem estava executando e não foi escolhida sofre preempção.
    for (TCB& tcb : novo.tcbs)
    {
        if (tcb.getEstado() == EstadoTarefa::executando && !escolhidas.count(&tcb))
        {
            logEventos.push_back({t, TipoEvento::preempcao, tcb.getTarefa().getId(), tcb.getCpu()});
            tcb.setEstado(EstadoTarefa::pronta);
            tcb.setCpu(-1);
        }
    }

    //    4b) Quem foi escolhida e já executava continua na MESMA CPU (evita
    //        migrações desnecessárias entre processadores).
    std::vector<TCB*> naCpu(static_cast<std::size_t>(numCpus), nullptr);
    for (TCB* p : res.escolhidas)
        if (p->getEstado() == EstadoTarefa::executando)
            naCpu[static_cast<std::size_t>(p->getCpu())] = p;

    //    4c) As demais escolhidas ocupam a CPU livre de menor índice. Ordenamos
    //        por id só para a atribuição ser previsível (id menor, CPU menor);
    //        a escolha de QUEM executa já foi feita pelo escalonador.
    std::vector<TCB*> novatas;
    for (TCB* p : res.escolhidas)
        if (p->getEstado() != EstadoTarefa::executando) novatas.push_back(p);
    std::sort(novatas.begin(), novatas.end(), [](const TCB* a, const TCB* b) {
        return a->getTarefa().getId() < b->getTarefa().getId();
    });

    for (TCB* p : novatas)
    {
        std::size_t c = 0;
        while (naCpu[c] != nullptr) c++;   // sempre existe CPU livre: escolhidas <= numCpus
        naCpu[c] = p;
        p->setEstado(EstadoTarefa::executando);
        p->setCpu(static_cast<int>(c));
        p->setTempoNoQuantum(0);
        // O gantt mostra o sorteio no momento em que a tarefa (re)começa a executar.
        if (sorteadas.count(p->getTarefa().getId()))
            logEventos.push_back({t, TipoEvento::sorteio, p->getTarefa().getId(), static_cast<int>(c)});
    }

    //    4d) CPU sem tarefa é desligada; ao voltar a ter tarefa, é ligada.
    std::vector<int> linha(static_cast<std::size_t>(numCpus), -1);
    for (std::size_t c = 0; c < naCpu.size(); c++)
    {
        if (naCpu[c] != nullptr)
        {
            linha[c] = naCpu[c]->getTarefa().getId();
            if (!novo.cpuLigada[c])
            {
                novo.cpuLigada[c] = true;
                logEventos.push_back({t, TipoEvento::cpuLigada, -1, static_cast<int>(c)});
            }
        }
        else if (novo.cpuLigada[c])
        {
            novo.cpuLigada[c] = false;
            logEventos.push_back({t, TipoEvento::cpuDesligada, -1, static_cast<int>(c)});
        }
    }
    tracoExec.push_back(linha);

    // 5) Executa 1 tick. Quem zerar o tempo restante termina em t+1.
    for (TCB* p : naCpu)
    {
        if (p == nullptr) continue;
        p->setTempoRestante(p->getTempoRestante() - 1);
        p->setTempoNoQuantum(p->getTempoNoQuantum() + 1);
        if (p->getTempoRestante() == 0)
        {
            logEventos.push_back({t + 1, TipoEvento::termino, p->getTarefa().getId(), p->getCpu()});
            p->concluirAtivacao();   // libera a CPU e passa para a próxima ativação, se houver
        }
    }

    novo.tick = t + 1;
    novo.numEventos = logEventos.size();
    historico.push_back(std::move(novo));
    cursor = historico.size() - 1;
}

// ---------------------------------------------------------------------------
bool Simulador::passo()
{
    // Já calculamos este futuro (o usuário tinha retrocedido): só reexibe.
    if (cursor + 1 < historico.size())
    {
        cursor++;
        return true;
    }
    if (terminou()) return false;
    if (historico[cursor].tick >= LIMITE_TICKS)
    {
        estourouLimite = true;
        return false;
    }
    calcularPasso();
    return true;
}

bool Simulador::voltar()
{
    if (cursor == 0) return false;
    cursor--;
    return true;
}

bool Simulador::executarCompleto()
{
    while (passo()) {}
    return terminou();
}

// ---------------------------------------------------------------------------
bool Simulador::terminou() const
{
    for (const TCB& t : historico[cursor].tcbs)
        if (t.getEstado() != EstadoTarefa::terminada) return false;
    return true;
}

bool Simulador::limiteExcedido() const { return estourouLimite; }
int Simulador::tickAtual() const { return historico[cursor].tick; }
const EstadoSistema& Simulador::estadoAtual() const { return historico[cursor]; }
const Configuracao& Simulador::configuracao() const { return config; }
unsigned int Simulador::getSemente() const { return semente; }
const std::vector<Evento>& Simulador::eventos() const { return logEventos; }
std::size_t Simulador::numEventosVisiveis() const { return historico[cursor].numEventos; }
const std::vector<std::vector<int>>& Simulador::traco() const { return tracoExec; }

int Simulador::ticksDesligada(int cpu) const
{
    int total = 0;
    for (std::size_t t = 0; t < cursor; t++)
        if (tracoExec[t][static_cast<std::size_t>(cpu)] == -1) total++;
    return total;
}

// ---------------------------------------------------------------------------
void Simulador::descartarFuturo()
{
    historico.erase(historico.begin() + static_cast<std::ptrdiff_t>(cursor) + 1, historico.end());
    tracoExec.erase(tracoExec.begin() + static_cast<std::ptrdiff_t>(cursor), tracoExec.end());
    logEventos.erase(logEventos.begin() + static_cast<std::ptrdiff_t>(historico[cursor].numEventos),
                     logEventos.end());
    estourouLimite = false;
}

TCB* Simulador::buscar(int id)
{
    for (TCB& t : historico[cursor].tcbs)
        if (t.getTarefa().getId() == id) return &t;
    return nullptr;
}

// ---------------------------------------------------------------------------
ResultadoEdicao Simulador::editarTarefa(int id, const std::string& campoOriginal, int valor)
{
    TCB* tcb = buscar(id);
    if (tcb == nullptr)
        return erro("A tarefa " + std::to_string(id) + " não existe.");
    if (tcb->getEstado() == EstadoTarefa::terminada)
        return erro("A tarefa " + std::to_string(id) + " já terminou; não há mais o que modificar.");

    const std::string campo = minusculas(campoOriginal);
    std::string nota;

    if (campo == "tempo_restante")
    {
        // Só faz sentido para quem tem uma ativação em andamento.
        if (!estaAtiva(*tcb))
            return erro("A tarefa " + std::to_string(id) + " está " + nomeEstado(tcb->getEstado()) +
                        ": não há ativação em andamento, então não existe tempo restante para alterar.");
        const int duracao = tcb->getTarefa().getDuracao();
        if (valor < 1 || valor > duracao)
            return erro("tempo_restante deve estar entre 1 e a duração da tarefa (" +
                        std::to_string(duracao) + "). Para encerrar a tarefa use 'encerrar'.");
        tcb->setTempoRestante(valor);
    }
    else if (campo == "duracao")
    {
        if (valor < 1) return erro("duracao deve ser um inteiro >= 1.");
        tcb->tarefaMutavel().setDuracao(valor);
        nota = "A nova duração vale a partir da PRÓXIMA ativação; a ativação em andamento "
               "mantém o tempo restante atual (altere-o com 'tempo_restante' se quiser).";
    }
    else if (campo == "prazo")
    {
        if (valor < 1) return erro("prazo deve ser um inteiro >= 1.");
        tcb->tarefaMutavel().setPrazo(valor);
        nota = "O novo prazo vale a partir da PRÓXIMA ativação; o prazo já calculado para a "
               "ativação em andamento não muda.";
    }
    else if (campo == "periodo")
    {
        if (valor < 1)
            return erro("periodo deve ser um inteiro >= 1 (periodo 0 ou negativo é de tarefa "
                        "aperiódica/esporádica, tratada só no Projeto B).");
        // Se já chegou alguma ativação, a próxima passa a ser "última + novo período".
        if (tcb->getAtivacoesLiberadas() > 0 &&
            tcb->getAtivacoesLiberadas() < TCB::ATIVACOES_MAXIMAS)
        {
            const int proxima = tcb->getUltimaLiberacao() + valor;
            if (proxima < tickAtual())
                return erro("Com periodo " + std::to_string(valor) + " a próxima ativação seria no "
                            "instante " + std::to_string(proxima) + ", que já passou (instante atual: " +
                            std::to_string(tickAtual()) + "). Escolha um período maior.");
            tcb->setInstanteProximaLiberacao(proxima);
        }
        tcb->tarefaMutavel().setPeriodo(valor);
        nota = "O novo período vale a partir da próxima ativação.";
    }
    else if (campo == "ingresso")
    {
        if (tcb->getAtivacoesLiberadas() > 0)
            return erro("A tarefa " + std::to_string(id) + " já chegou ao sistema; o ingresso só pode "
                        "ser alterado antes da primeira ativação.");
        if (valor < tickAtual())
            return erro("O ingresso não pode ser anterior ao instante atual (" +
                        std::to_string(tickAtual()) + ").");
        tcb->tarefaMutavel().setIngresso(valor);
        tcb->setInstanteProximaLiberacao(valor);
    }
    else
    {
        return erro("Campo '" + campoOriginal + "' desconhecido. Campos editáveis: "
                    "tempo_restante, duracao, periodo, prazo, ingresso.");
    }

    descartarFuturo();
    return sucesso(nota);
}

ResultadoEdicao Simulador::encerrarTarefa(int id)
{
    TCB* tcb = buscar(id);
    if (tcb == nullptr)
        return erro("A tarefa " + std::to_string(id) + " não existe.");
    if (tcb->getEstado() == EstadoTarefa::terminada)
        return erro("A tarefa " + std::to_string(id) + " já está terminada.");

    const int cpu = tcb->getCpu();
    tcb->encerrar();
    descartarFuturo();
    // Registra o fim no instante atual. Se o futuro foi descartado, o log
    // termina exatamente nos eventos já ocorridos, então basta acrescentar.
    logEventos.push_back({tickAtual(), TipoEvento::termino, id, cpu});
    historico[cursor].numEventos = logEventos.size();
    return sucesso("A tarefa foi encerrada no instante " + std::to_string(tickAtual()) + ".");
}

ResultadoEdicao Simulador::alterarAlgoritmo(const std::string& nome)
{
    std::unique_ptr<Escalonador> novo = criarEscalonador(nome);
    if (!novo)
        return erro("Algoritmo '" + nome + "' desconhecido. Algoritmos disponíveis: RM, EDF.");
    escalonador = std::move(novo);
    config.setAlgoritmo(escalonador->nome());
    descartarFuturo();
    return sucesso("O novo algoritmo vale a partir do instante " + std::to_string(tickAtual()) +
                   "; o que já foi simulado antes disso não é recalculado.");
}

ResultadoEdicao Simulador::alterarNumCpus(int numCpus)
{
    if (numCpus < 1)
        return erro("A quantidade de CPUs deve ser pelo menos 1.");
    // Mudar o número de CPUs no meio da simulação deixaria o gantt dos
    // instantes anteriores inconsistente (CPUs que "apareceriam" do nada).
    if (tickAtual() != 0)
        return erro("A quantidade de CPUs só pode ser alterada no instante 0 (você está no instante " +
                    std::to_string(tickAtual()) + "). Volte ao início da simulação para alterá-la.");
    config.setNumCpus(numCpus);
    descartarFuturo();
    historico[cursor].cpuLigada.assign(static_cast<std::size_t>(numCpus), true);
    return sucesso();
}
