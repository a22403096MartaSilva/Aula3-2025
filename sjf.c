#include "sjf.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>

/**
 * @brief Algoritmo de escalonamento Shortest-Job-First (SJF) não-preemptivo.
 *
 * Esta função implementa o algoritmo SJF.
 * - Se já existe uma tarefa no CPU, soma o tempo de execução e verifica se terminou.
 * - Se o CPU estiver livre, escolhe, entre as tarefas que estão à espera,
 *   aquela que precisa de menos tempo para acabar.
 *
 * O tempo que falta para cada tarefa é:
 *    tempo_restante = tempo_total - tempo_executado
 *
 * Regras:
 * - Se duas tarefas tiverem o mesmo tempo restante, é escolhida a que entrou
 *   primeiro na fila.
 * - O SJF é não-preemptivo: depois de começar, a tarefa só larga o CPU quando
 *   terminar ou pedir E/S.
 * - Quando uma tarefa regressa de E/S, volta à lista de espera e é escolhida
 *   de acordo com o tempo que ainda lhe falta.
 *
 * @param current_time_ms Tempo actual em milissegundos.
 * @param rq Lista de espera com as tarefas que ainda não estão a correr.
 * @param cpu_task Ponteiro para a tarefa que está no CPU; será actualizado
 *                 para a próxima tarefa escolhida.
 */





void sjf_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // 1) Se há tarefa no CPU, avança o trabalho deste tick
    if (*cpu_task) {
        (*cpu_task)->ellapsed_time_ms += TICKS_MS;

        // 2) Se terminou, notifica e liberta
        if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
            msg_t msg = {
                .pid = (*cpu_task)->pid,
                .request = PROCESS_REQUEST_DONE,
                .time_ms = current_time_ms
            };
            if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t)) {
                perror("write");
            }
            free(*cpu_task);
            *cpu_task = NULL;
        }
    }

    // 3) Se o CPU está livre, escolhe o processo com menor tempo total (SJF)
    if (*cpu_task == NULL) {
        queue_elem_t *prev = NULL;
        queue_elem_t *prev_shortest = NULL;
        queue_elem_t *current = rq->head;
        queue_elem_t *shortest_elem = NULL;

        // percorre a fila para encontrar o PCB com menor time_ms
        while (current != NULL) {
            if (shortest_elem == NULL ||
                current->pcb->time_ms < shortest_elem->pcb->time_ms) {
                shortest_elem = current;
                prev_shortest = prev;
                }
            prev = current;
            current = current->next;
        }

        // remove o elemento mais curto da fila
        if (shortest_elem != NULL) {
            if (prev_shortest == NULL) {
                // é o primeiro da fila
                rq->head = shortest_elem->next;
            } else {
                prev_shortest->next = shortest_elem->next;
            }

            if (shortest_elem == rq->tail) {
                rq->tail = prev_shortest;
            }

            // entrega o PCB ao CPU e liberta o nó
            *cpu_task = shortest_elem->pcb;
            free(shortest_elem);
        }
    }
}


