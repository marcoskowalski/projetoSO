//
// Created by mvmko on 20/10/2026.
//

#ifndef PROJETOSO_TAREFA_H
#define PROJETOSO_TAREFA_H


class Tarefa
{
     private:
        int id;
        int periodo;
        int ingresso;
        int duracao;
        int prazo;
        unsigned int cor;
    public:
        Tarefa(int id, int periodo, int ingresso, int duracao, int prazo, int cor);
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