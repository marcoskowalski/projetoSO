//
// Created by mvmko on 19/10/2026.
//

#ifndef PROJETOSO_ESTADOTAREFA_H
#define PROJETOSO_ESTADOTAREFA_H

// Estados possíveis de uma tarefa.
//
// Por que existe "inativa"? No PDF (item 2.1), "suspensa" significa tarefa
// bloqueada (p.ex. esperando um mutex) e é desenhada em preto/pontilhado no
// gráfico de gantt. Uma tarefa periódica que ainda não chegou, ou que já
// terminou a ativação atual e espera a próxima, NÃO está suspensa: ela
// simplesmente não tem nada para executar. Usar "suspensa" para isso faria o
// gantt mostrar bloqueios que não existem.
enum class EstadoTarefa
{
        inativa,     // sem ativação em andamento (ainda não chegou / esperando o próximo período)
        pronta,      // ativada, na fila de prontos, esperando uma CPU
        executando,  // ocupando uma CPU neste instante
        suspensa,    // bloqueada (reservado ao Projeto B: mutex, E/S...)
        terminada    // já executou todas as ativações previstas (10 no projeto A)
};
#endif //PROJETOSO_ESTADOTAREFA_H
