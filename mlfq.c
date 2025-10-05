#include "mlfq.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "msg.h"

#define NUM_QUEUES 3
#define NUM_QUEUES   3
#define Q0_SLICE_MS  500
#define Q1_SLICE_MS  1000
#define Q2_SLICE_MS  2000

static inline uint32_t slice_of(int lvl) {
    if (lvl == 0) return Q0_SLICE_MS;
    if (lvl == 1) return Q1_SLICE_MS;
    return Q2_SLICE_MS;
}

void mlfq_scheduler(uint32_t current_time_ms, queue_t *rq, pcb_t **cpu_task) {
    // 3 filas internas persistentes (alta, média, baixa)
    static queue_t q[NUM_QUEUES] = {
        {.head=NULL,.tail=NULL},
        {.head=NULL,.tail=NULL},
        {.head=NULL,.tail=NULL}
    };
    // nível da tarefa que está *neste momento* no CPU
    static int running_level = 0;

    // 1) Tudo o que entrou na ready_queue global vai para Q0
    while (rq->head) {
        pcb_t *p = dequeue_pcb(rq);
        enqueue_pcb(&q[0], p);
    }

    // 2) Se há alguém a correr, avança tempo e aplica regras
    if (*cpu_task) {
        pcb_t *p = *cpu_task;
        p->ellapsed_time_ms += TICKS_MS;

        // a) terminou?
        if (p->ellapsed_time_ms >= p->time_ms) {
            msg_t msg = { .pid=p->pid, .request=PROCESS_REQUEST_DONE, .time_ms=current_time_ms };
            if (write(p->sockfd, &msg, sizeof msg) != sizeof msg) perror("write");
            free(p);
            *cpu_task = NULL;
        }
        // b) esgotou o time-slice do nível atual? -> desce de fila
        else if ((p->ellapsed_time_ms - p->slice_start_ms) >= slice_of(running_level)) {
            if (running_level < NUM_QUEUES - 1) running_level++;   // desce (até Q2)
            p->slice_start_ms = p->ellapsed_time_ms;               // reinicia início do slice
            enqueue_pcb(&q[running_level], p);
            *cpu_task = NULL;
        }
    }

    // 3) CPU livre? escolhe da fila mais prioritária disponível
    if (*cpu_task == NULL) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            *cpu_task = dequeue_pcb(&q[i]);
            if (*cpu_task) {
                running_level = i;  // sabemos o nível porque veio desta fila
                (*cpu_task)->slice_start_ms = (*cpu_task)->ellapsed_time_ms;
                break;
            }
        }
    }
}

