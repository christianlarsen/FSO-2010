
// Ultima version: 01122010 11.21


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>

#include "LTareas.h"

void LT_Inicializa(TAREAS *jobs) {
    // Inicializa la lista de tareas.
    jobs->ntareas = 0;
    jobs->inicio = NULL;
}

void LT_BorrarTareas(TAREAS *jobs) {
    // Borra la lista de tareas y libera la memoria...
    int k;

    if (jobs->ntareas > 0) {
        TAREA *job, *borrajob;
        job = jobs->inicio;

        for (k = 1; k <= jobs->ntareas; k++) {
            borrajob = job;
            job = job->next;
            free(borrajob->command);
            free(borrajob);
        }
        free(jobs);
    } else {
        free(jobs);
    }
}

void LT_AnadeTrabajo(TAREAS *jobs, pid_t fork_pid, char *trabajo, int bg, struct termios tm) {
    // Añade el trabajo a la lista de tareas con el numero de pid, nombre trabajo y el codigo de trabajo segun bg.

    // creo un trabajo
    TAREA *job = malloc(sizeof (TAREA));
    job->command = malloc(sizeof ((char *) trabajo));
    strcpy(job->command, trabajo);
    job->foreground = 0;
    job->modificado = 0;
    job->pgid = fork_pid;
    job->status = 0;
    job->stopped = 0;
    if (bg == 2) // tarea de primer plano
        job->foreground = 1;
    job->tmodes = tm;
    // y lo añado a la lista.
    jobs->ntareas++;
    job->next = jobs->inicio;
    jobs->inicio = job;
}

void LT_SacaTrabajo(pid_t pid, TAREAS *jobs) {
    // Saca de la lista de tareas el trabajo dado por el pid.
    // Siempre que llamo a esta funcion hay al menos una tarea... asi que no miro los posibles errores provocados
    // por la inexistencia de tareas en la lista...

    int i, nt;
    char *t;

    TAREA *actual, *anterior;
    TAREA *sup_elemento = malloc(sizeof (TAREA));

    actual = jobs->inicio;
    anterior = NULL;
    nt = jobs->ntareas;

    if (nt == 1) // unicamente elimino la cabeza de la lista...
    {
        free(jobs->inicio->command);
        free(jobs->inicio);
        jobs->inicio = NULL;
        jobs->ntareas--;
        

    } else {

        for (i = 1; i <= nt; i++) {
            if (actual->pgid == pid) break;
            anterior = actual;
            actual = actual->next;

        }
        if (i == 1) // estamos en la cabeza de la lista...
        {
            sup_elemento = jobs->inicio;
            jobs->inicio = jobs->inicio->next;
            jobs->ntareas--;
            free(sup_elemento);
        } else {
            // estamos en otro lugar de la lista...
            sup_elemento = actual;
            anterior->next = actual->next;

            free(sup_elemento->command);
            free(sup_elemento);
            jobs->ntareas--;
        }
        
    }

}

void LT_ImprimeTrabajos(TAREAS *jobs, int tipotrabajo) {
    // Imprime los trabajos que tenemos en la lista, precedidos de su número de orden en la misma [1], [2], ...

    TAREA *j;
    int k;

    j = jobs->inicio;
    if (jobs->ntareas == 0) {
        printf(":: No hay tareas en ejecución.\n");
        return;
    } else {
        for (k = 1; k <= jobs->ntareas; k++) {

            // si tipotrabajo = 1 entonces queremos tareas en segundo plano...
            if (tipotrabajo == 1)
                if (j->stopped  == 0)
                    printf(":: [%d]: Trabajo %s, pid=%d, ejecutandose en segundo plano.\n", k, (char *) j->command, j->pgid);
                else if (j->stopped == 1)
                    printf(":: [%d]: Trabajo %s, pid=%d, en suspensión.\n", k, (char *) j->command, j->pgid);
            j = j->next;
        }
    }

}

void LT_CambiaTareaSusp(pid_t pid, TAREAS *jobs) {

    // Cambio el estado de una tarea en la lista jobs  por suspendida...
    // siempre llamaré a esta funcion cuando la tarea está ejecutandose en primer plano...

    TAREA *j;
    int k;

    j = jobs->inicio;
    for (k = 1; k <= jobs->ntareas; k++) {
        // busco el trabajo...
        if (j->pgid == pid) break;
        else j = j->next;
    }
    j->foreground = 0;
    j->stopped = 1;
    
}

void LT_CambiaTareaFg(TAREAS *jobs,pid_t pid) {

    TAREA *j;
    int k;

    j = jobs->inicio;
    for (k = 1; k <= jobs->ntareas; k++) {
        // busco el trabajo...
        if (j->pgid == pid) break;
        else j = j->next;
    }
    j->foreground = 1;
    j->stopped = 0;
}

void LT_CambiaTareaBg(TAREAS *jobs,pid_t pid) {

    TAREA *j;
    int k;

    j = jobs->inicio;
    for (k = 1; k <= jobs->ntareas; k++) {
        // busco el trabajo...
        if (j->pgid == pid) break;
        else j = j->next;
    }
    j->foreground = 0;
    j->stopped = 0;
}

void LT_CambiaTareaSegPlano(pid_t pid, TAREAS *jobs) {

    // Cambio el estado de una tarea en la lista jobs  por de segundo plano...

    TAREA *j;
    int k;

    j = jobs->inicio;
    for (k = 1; k <= jobs->ntareas; k++) {
        // busco el trabajo...
        if (j->pgid == pid) break;
        else j = j->next;
    }
    j->foreground = 0;
    j->modificado = 0;
    j->status = 0;
    j->stopped = 0;

}

void LT_CambiaStatusTarea(TAREAS *jobs, pid_t pid , int status) {
    TAREA *job;

    if (jobs == NULL) return;
    else {

        job = jobs->inicio;

        // buscar el proceso a actualizar
        while ((job!= NULL) && (job->pgid != pid)) {
            job = job->next;
        }

        if (job != NULL) job->status = status;
    }
}