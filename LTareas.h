/* 
 * File:   LTareas.h
 * Author: christian
 *
 */

// Ultima version: 01122010 11.21

#ifndef LTAREAS_H
#define	LTAREAS_H

#ifdef	__cplusplus
extern "C" {
#endif




#ifdef	__cplusplus
}
#endif

#endif	/* LTAREAS_H */

/* Estructura para control de tareas */
typedef struct untrabajo {
    pid_t pgid; /* pid del grupo = pid del proceso lider */
    char * command; /* Nombre del programa del proceso */
    int stopped; /* Cierto si el proceso está parado(suspendido)*/
    int status; /* Valor devuelto por status(en waitpid)*/
    int modificado; /* modificado estatus pero aún no informado */
    int foreground; /* Cierto si proceso está en primer plano */
    struct termios tmodes; /* Guarda los modos del terminal */
    struct untrabajo *next; /* apunta a la estrucura job siquiente */
} TAREA;

typedef struct trabajos {
    int ntareas;
    TAREA *inicio;
} TAREAS;

/* Funciones para el uso de la lista de tareas */

void LT_Inicializa(TAREAS *jobs);

void LT_BorrarTareas(TAREAS *jobs);

void LT_AnadeTrabajo(TAREAS *jobs, pid_t fork_pid, char *trabajo, int bg, struct termios tm);

void LT_SacaTrabajo(pid_t pid,TAREAS *jobs);

void LT_EjecutandoTrabajo(TAREAS *jobs, pid_t pid);

void LT_CambiaTareaSusp(pid_t pid, TAREAS *jobs);

void LT_CambiaTareaFg(TAREAS *jobs,pid_t pid);

void LT_CambiaTareaBg(TAREAS *jobs,pid_t pid);

void LT_CambiaTareaSegPlano(pid_t pid, TAREAS *jobs);

void LT_CambiaStatusTarea(TAREAS *jobs, pid_t pid, int status);