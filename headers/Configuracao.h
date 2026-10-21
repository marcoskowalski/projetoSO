//
// Created by mvmko on 20/10/2026.
//

#ifndef PROJETOSO_CONFIGURACAO_H
#define PROJETOSO_CONFIGURACAO_H

#include <string>


class Configuracao{
    private:
        int quantum;
        int numCpus;
        std::string algoritmo;
    public:
        Configuracao(int quantum, int numCpus, std::string algoritmo);
        Configuracao();
        ~Configuracao();

        [[nodiscard]] int getQuantum() const;
        [[nodiscard]] int getNumCpus() const;
        [[nodiscard]] std::string getAlgoritmo() const;

        void setQuantum(int quantum);
        void setNumCpus(int numCpus);
        void setAlgoritmo(std::string algoritmo);


};


#endif //PROJETOSO_CONFIGURACAO_H