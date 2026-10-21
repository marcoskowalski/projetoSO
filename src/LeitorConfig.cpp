#include "../headers/LeitorConfig.h"

#include <cctype>
#include <charconv>
#include <fstream>

// ---------------------------------------------------------------------------
// Funções auxiliares (visíveis só neste arquivo)
// ---------------------------------------------------------------------------
namespace
{
    // Remove espaços (e '\r' de arquivos do Windows) no início e no fim.
    std::string trim(const std::string& s)
    {
        std::size_t ini = 0;
        std::size_t fim = s.size();
        while (ini < fim && std::isspace(static_cast<unsigned char>(s[ini]))) ini++;
        while (fim > ini && std::isspace(static_cast<unsigned char>(s[fim - 1]))) fim--;
        return s.substr(ini, fim - ini);
    }

    // Converte para maiúsculas: "rm", "Rm" e "RM" viram a mesma coisa.
    std::string maiuscula(std::string s)
    {
        for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }

    // Separa a linha nos ';', mantendo campos vazios e aplicando trim neles.
    std::vector<std::string> dividir(const std::string& linha)
    {
        std::vector<std::string> campos;
        std::size_t ini = 0;
        while (true)
        {
            std::size_t pos = linha.find(';', ini);
            if (pos == std::string::npos)
            {
                campos.push_back(trim(linha.substr(ini)));
                break;
            }
            campos.push_back(trim(linha.substr(ini, pos - ini)));
            ini = pos + 1;
        }
        return campos;
    }

    // Converte o texto INTEIRO em int; retorna false se tiver lixo ou estourar.
    bool paraInt(const std::string& s, int& valor)
    {
        if (s.empty()) return false;
        const char* fim = s.data() + s.size();
        auto [p, ec] = std::from_chars(s.data(), fim, valor);
        return ec == std::errc() && p == fim;
    }

    // Cor válida = exatamente 6 dígitos hexadecimais (RRGGBB).
    bool corValida(const std::string& s)
    {
        if (s.size() != 6) return false;
        for (char c : s)
            if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
        return true;
    }

    std::string prefixo(int numLinha)
    {
        return "Linha " + std::to_string(numLinha) + ": ";
    }
}

LeitorConfig::LeitorConfig() = default;
LeitorConfig::~LeitorConfig() = default;

// ---------------------------------------------------------------------------
bool LeitorConfig::carregar(const std::string& caminho)
{
    // Começa do zero, caso o mesmo leitor seja usado para vários arquivos.
    configuracao = Configuracao();
    tarefas.clear();
    erros.clear();
    avisos.clear();
    idsLidos.clear();

    std::ifstream arquivo(caminho);
    if (!arquivo)
    {
        erros.push_back("Não foi possível abrir o arquivo '" + caminho +
                        "'. Verifique o caminho e a permissão de leitura.");
        return false;
    }

    std::string linha;
    int numLinha = 0;
    bool leuCabecalho = false;

    while (std::getline(arquivo, linha))
    {
        numLinha++;
        // Alguns editores do Windows põem um BOM (3 bytes) no início do arquivo.
        if (numLinha == 1 && linha.rfind("\xEF\xBB\xBF", 0) == 0) linha.erase(0, 3);

        linha = trim(linha);
        if (linha.empty()) continue;            // linhas em branco são ignoradas

        if (!leuCabecalho)
        {
            leuCabecalho = true;                // 1ª linha não vazia = parâmetros do sistema
            lerCabecalho(linha, numLinha);
        }
        else
        {
            lerTarefa(linha, numLinha);
        }
    }

    if (!leuCabecalho)
        erros.push_back("O arquivo está vazio: faltam os parâmetros do sistema e as tarefas.");
    else if (tarefas.empty() && erros.empty())
        erros.push_back("O arquivo não tem nenhuma tarefa periódica válida.");

    return erros.empty();
}

// ---------------------------------------------------------------------------
void LeitorConfig::lerCabecalho(const std::string& linha, int numLinha)
{
    std::vector<std::string> c = dividir(linha);

    // Um ';' no final da linha gera um campo vazio extra: é permitido.
    if (c.size() > 3 && c.back().empty()) c.pop_back();
    if (c.size() > 3)
    {
        erros.push_back(prefixo(numLinha) + "esperado 'algoritmo;quantum;qtde_cpus', "
                        "mas a linha tem campos demais.");
        return;
    }
    c.resize(3);   // campos que faltaram viram vazios e recebem o valor padrão

    // --- algoritmo (padrão: EDF) ---
    std::string algoritmo = maiuscula(c[0]);
    if (algoritmo.empty())
    {
        algoritmo = "EDF";
        avisos.push_back(prefixo(numLinha) + "algoritmo ausente; usando EDF.");
    }
    else if (algoritmo != "RM" && algoritmo != "EDF")
    {
        erros.push_back(prefixo(numLinha) + "algoritmo '" + c[0] + "' desconhecido (use RM ou EDF).");
    }

    // --- quantum (padrão: 1) ---
    int quantum = 1;
    if (c[1].empty())
        avisos.push_back(prefixo(numLinha) + "quantum ausente; usando 1.");
    else if (!paraInt(c[1], quantum) || quantum < 1)
    {
        erros.push_back(prefixo(numLinha) + "quantum '" + c[1] + "' inválido (inteiro >= 1).");
        quantum = 1;
    }

    // --- qtde_cpus (padrão: 1) ---
    int numCpus = 1;
    if (c[2].empty())
        avisos.push_back(prefixo(numLinha) + "qtde_cpus ausente; usando 1.");
    else if (!paraInt(c[2], numCpus) || numCpus < 1)
    {
        erros.push_back(prefixo(numLinha) + "qtde_cpus '" + c[2] + "' inválida (inteiro >= 1).");
        numCpus = 1;
    }

    configuracao.setAlgoritmo(algoritmo);
    configuracao.setQuantum(quantum);
    configuracao.setNumCpus(numCpus);
}

// ---------------------------------------------------------------------------
void LeitorConfig::lerTarefa(const std::string& linha, int numLinha)
{
    std::vector<std::string> c = dividir(linha);

    // Precisa de id;cor;ingresso;duracao;periodo;prazo. O 7º campo
    // (lista_eventos) é do Projeto B e é simplesmente ignorado aqui.
    if (c.size() < 6)
    {
        erros.push_back(prefixo(numLinha) + "a tarefa precisa de 6 campos "
                        "(id;cor;ingresso;duracao;periodo;prazo), mas tem " +
                        std::to_string(c.size()) + ".");
        return;
    }

    const std::size_t errosAntes = erros.size();

    int id = 0, ingresso = 0, duracao = 0, periodo = 0, prazo = 0;
    unsigned int cor = 0;

    // --- id (inteiro, único) ---
    // Vale para qualquer tarefa, inclusive as que serão ignoradas abaixo:
    // os ids do arquivo precisam ser únicos de qualquer jeito.
    bool idOk = true;
    if (!paraInt(c[0], id) || id < 0)
    {
        erros.push_back(prefixo(numLinha) + "id '" + c[0] + "' inválido (inteiro >= 0).");
        idOk = false;
    }
    else if (idsLidos.count(id))
    {
        erros.push_back(prefixo(numLinha) + "id " + std::to_string(id) + " repetido.");
        idOk = false;
    }

    // --- periodo é lido ANTES dos outros campos ---
    // Uma tarefa aperiódica (periodo 0) só existe no Projeto B. No Projeto A
    // ela deve ser IGNORADA com um aviso (requisito 4.4), então o resto da
    // linha (prazo, duração...) não pode gerar erro: antes, "3;0000FF;0;1;0;0"
    // era rejeitado porque o prazo 0 era validado antes desta checagem.
    const bool periodoOk = paraInt(c[4], periodo);
    if (periodoOk && periodo == 0)
    {
        if (idOk)
        {
            idsLidos.insert(id);   // o id continua "ocupado", mesmo ignorada
            avisos.push_back(prefixo(numLinha) + "tarefa " + std::to_string(id) +
                             " é aperiódica (periodo 0) e foi ignorada (só é tratada no Projeto B).");
        }
        return;
    }

    if (!periodoOk)
        erros.push_back(prefixo(numLinha) + "periodo '" + c[4] + "' inválido (inteiro).");
    else if (periodo < 0)
        erros.push_back(prefixo(numLinha) + "periodo negativo é erro no arquivo (periodo = " +
                        c[4] + ").");

    // --- cor (RRGGBB em hexadecimal) ---
    if (!corValida(c[1]))
        erros.push_back(prefixo(numLinha) + "cor '" + c[1] + "' inválida (use 6 dígitos hexadecimais, ex.: F0E0D0).");
    else
        std::from_chars(c[1].data(), c[1].data() + 6, cor, 16);

    // --- demais campos numéricos ---
    if (!paraInt(c[2], ingresso) || ingresso < 0)
        erros.push_back(prefixo(numLinha) + "ingresso '" + c[2] + "' inválido (inteiro >= 0).");
    if (!paraInt(c[3], duracao) || duracao < 1)
        erros.push_back(prefixo(numLinha) + "duracao '" + c[3] + "' inválida (inteiro >= 1).");

    // prazo vazio: usa o próprio período (prazo implícito).
    if (c[5].empty())
        prazo = periodo;
    else if (!paraInt(c[5], prazo) || prazo < 1)
        erros.push_back(prefixo(numLinha) + "prazo '" + c[5] + "' inválido (inteiro >= 1).");

    if (erros.size() != errosAntes) return;   // não adianta validar o resto

    if (duracao > prazo)
        avisos.push_back(prefixo(numLinha) + "tarefa " + std::to_string(id) +
                         ": duracao maior que o prazo, vai perder o deadline.");

    idsLidos.insert(id);
    tarefas.emplace_back(id, periodo, ingresso, duracao, prazo, cor);
}

// ---------------------------------------------------------------------------
Configuracao LeitorConfig::getConfiguracao() const { return configuracao; }
std::vector<Tarefa> LeitorConfig::getTarefas() const { return tarefas; }
std::vector<std::string> LeitorConfig::getErros() const { return erros; }
std::vector<std::string> LeitorConfig::getAvisos() const { return avisos; }
