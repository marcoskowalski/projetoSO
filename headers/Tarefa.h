//
// Created by mvmko on 20/10/2026.
//

#ifndef PROJETOSO_TAREFA_H
#define PROJETOSO_TAREFA_H

// Parâmetros ESTÁTICOS de uma tarefa, exatamente como vieram do arquivo de
// configuração. O estado que muda durante a simulação (tempo restante,
// prazo absoluto, CPU...) fica no TCB, não aqui.
//
// Os membros têm valor inicial para que o construtor padrão nunca deixe lixo
// de memória (antes, Tarefa() = default deixava os campos sem inicializar).
class Tarefa
{
     private:
        int id = 0;
        int periodo = 0;
        int ingresso = 0;
        int duracao = 0;
        int prazo = 0;
        unsigned int cor = 0;   // 0xRRGGBB
    public:
        Tarefa(int id, int periodo, int ingresso, int duracao, int prazo, unsigned int cor);
        Tarefa();
        ~Tarefa();

        [[nodiscard]] int getId() const;
        [[nodiscard]] int getPeriodo() const;
        [[nodiscard]] int getIngresso() const;
        [[nodiscard]] int getDuracao() const;
        [[nodiscard]] int getPrazo() const;
        [[nodiscard]] unsigned int getCor() const;

        void setPeriodo(int periodo);
        void setIngresso(int ingresso);
        void setDuracao(int duracao);
        void setPrazo(int prazo);
        void setCor(unsigned int cor);

};


#endif //PROJETOSO_TAREFA_H
