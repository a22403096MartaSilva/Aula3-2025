#include "rr.h"

#include <stdio.h>
#include <stdlib.h>

#include "msg.h"
#include <unistd.h>

/**
 * @brief RR (Round-Robin), time-slice = 500 milisegundos.
 *
* Este função implementa o algoritmo de escalonamento Round Robin com um time slice de 500ms.
 * Se a CPU não estiver livre, verifica se a tarefa em execução terminou ou se já esgotou
 * o seu time slice de 500ms. Se alguma destas condições se verificar, liberta a CPU.
 * Se a tarefa não terminou mas esgotou o time slice, é colocada no fim da fila de espera.
 * Se a CPU estiver livre, seleciona a próxima tarefa a executar baseando-se na ordem da fila.
 * Cada tarefa executa por um máximo de 500ms antes de dar lugar à próxima tarefa na fila.
 *
 * @param current_time_ms The current time in milliseconds.
 * @param rq Pointer to the ready queue containing tasks that are ready to run.
 * @param cpu_task Double pointer to the currently running task. This will be updated
 *                 to point to the next task to run.
 *
 */
void rr_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
 // 1) Se há tarefa a executar
 if (*cpu_task) {
  (*cpu_task)->ellapsed_time_ms += TICKS_MS;

  // a) Terminou completamente?
  if ((*cpu_task)->ellapsed_time_ms >= (*cpu_task)->time_ms) {
   msg_t msg = {
    .pid = (*cpu_task)->pid,
    .request = PROCESS_REQUEST_DONE,
    .time_ms = current_time_ms
};
   if (write((*cpu_task)->sockfd, &msg, sizeof(msg_t)) != sizeof(msg_t))
    perror("write");

   free(*cpu_task);
   *cpu_task = NULL;
  }

  // b) Esgotou o time slice (500 ms)?
  else if (((*cpu_task)->ellapsed_time_ms - (*cpu_task)->slice_start_ms) >= 500) {
   (*cpu_task)->slice_start_ms = (*cpu_task)->ellapsed_time_ms;
   enqueue_pcb(rq, *cpu_task);   // volta ao fim da fila
   *cpu_task = NULL;
  }
 }

 // 2) Se o CPU está livre, escolhe o próximo processo
 if (*cpu_task == NULL) {
  *cpu_task = dequeue_pcb(rq);
  if (*cpu_task) {
   (*cpu_task)->slice_start_ms = (*cpu_task)->ellapsed_time_ms;
  }
 }
}
