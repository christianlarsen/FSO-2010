/***  Shell basico con cesion del terminal
      Fundamentos de Sistemas Operativos
 ***/
/* Ultima versión:  01122010 / 11.21*/
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

#define MAXLINEA 1024  /* tamaño maximo linea comandos */
#define MAXARG 256     /* numero maximo argumentos linea comm. */
#define PROMPT "FSO"   /* prompt del shell */
#define VER "1.0112"   // versión de shell


/*============================================================*/
// variables globales
pid_t shell_pgid; // pgid del shell == pid del shell
struct termios shell_tmodes; // atributos del terminal para el shell
int shell_terminal; // descriptor de fichero del terminal
int pcomplt = 0; // tipo de propmt.

sigset_t block_sigchld;
TAREAS *jobs;


int help() {
    printf("   --------------------------------\n");
    printf("   Ayuda\n");
    printf("   --------------------------------\n");
    printf("   help                   ayuda\n");
    printf("   logout                 salir \n");
    printf("   cd                     cambio directorio \n");
    printf("   prompt                 cambiar tipo de prompt \n");
    printf("   programa <args>        ejecuta programa \n");
    printf("   programa <args> &      ejecuta programa en segundo plano \n");
    printf("   jobs                   devuelve una lista de procesos en segundo plano y suspendidos \n");
    printf("   fg <arg>               manda a primer plano el proceso dado en el argumento, si existe \n");
    printf("   ver                    nos devuelve la versión del shell \n");
    printf("   cls                    borra la pantalla \n");
    printf("\n");
}

int intro() {
    printf("   --------------------------------\n");
    printf("   Shell3B 2010 %s Bienvenido!!\n", VER);
    printf("   Control de de tareas FSO 2010-11\n");
    printf("   --------------------------------\n");
    printf("   help                   ayuda\n");
    printf("   logout                 salir \n");
    printf("\n");
}

int nueva_linea(char * linea, int len) {
    int i = 0;
    while (i < len) {
        if ((linea[i] = (char) fgetc(stdin)) == -1) {
            perror("fgetc");
            exit(-1);
        }

        if (linea[i] == '\n') {
            linea[i] = '\0';
            return (i);
        }
        i++;
        if (i == len) linea[i] = '\0';
    }
    return (i);
}

int lee_linea(char * linea, char **argumentos) {
    char *token;
    int narg = 0;

    if ((token = strtok(linea, " ")) != NULL) /* busca una palabra separada por espacios */ {
        argumentos[narg] = (char *) strdup(token); /* copia la palabra  */
        narg++; /* incrementa el contador de palabras */
        while ((token = strtok(NULL, " ")) != NULL) /* busca mas palabras en la misma linea */ {
            argumentos[narg] = (char *) strdup(token);
            narg++;
        }
    }
    argumentos[narg] = NULL;
    return narg;
}

int libera_mem_arg(char ** argumentos, int n_arg) {
    int i;
    for (i = 0; i < n_arg; i++)
        free(argumentos[i]);
    return;
}

void pinta_estado_tarea(int wait_pid, char * comando, int status) {
    if (WIFSTOPPED(status)) {
        // el proceso se suspendio
        // modifico la lista de tareas...
        LT_CambiaStatusTarea(jobs, wait_pid, status);
        LT_CambiaTareaSusp(wait_pid, jobs);
        // muestro el mensaje por pantalla...
        fprintf(stderr, "\n:: Suspendido trabajo %s [pid:%d], signal %d .\n",
                comando, (int) wait_pid, WSTOPSIG(status));

    } else {
        // el proceso terminó
        if (WIFSIGNALED(status))
            fprintf(stderr, "\n:: Terminado trabajo %s [pid:%d], signal %d.\n",
                comando, (int) wait_pid, WTERMSIG(status));
        else
            fprintf(stderr, "\n:: Terminado trabajo %s [pid:%d], estado %d.\n",
                comando, (int) wait_pid, WEXITSTATUS(status));
        // y sacamos el proceso de la lista de procesos...
        LT_SacaTrabajo(wait_pid,jobs);
    }
    
}

int cd(char **argumentos, int narg) {
    if (narg == 2) {
        if (chdir(argumentos[1])) {
            perror("chdir");
        }
    }
}

void manejador_controlc(int senal) {
    fprintf(stderr, "\n:: Utilice el comando 'logout' para salir.\n");
}

void manejador(int senal) {

    // Lo primero que hacemos es bloquear la señal...

    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);

    if (jobs->ntareas > 0)
    {
        // no vacia
        TAREA *job;
        int status;
        pid_t pid, wpid;

        job = jobs->inicio;

        do {
            // vemos si el proceso está en segundo plano o parado
            if ((!job->foreground) || (job->stopped)) {
                pid = job->pgid;
                wpid = waitpid(pid, &status, WNOHANG | WUNTRACED);
                if (wpid == -1) {
                    // error ...
                    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
                    perror("waitpid");
                }
                if (wpid == pid) {
                    if (WIFEXITED(status)) {
                        // El proceso ha terminado...
                        fprintf(stderr, "\n:: [pid=%d] : Proceso en segundo plano terminado.\n", wpid);
                        LT_SacaTrabajo(pid,jobs);
                    } else if (WIFSTOPPED(status)) {
                        // El proceso está suspendido...
                        fprintf(stderr, "\n:: [pid=%d] : Proceso en segundo plano suspendido.\n", wpid);
                        LT_CambiaStatusTarea(jobs, pid, status);
                        LT_CambiaTareaSusp(pid,jobs);
                    }
                    sigprocmask(SIG_UNBLOCK, &block_sigchld,NULL);

                    return;
                }
            }
            job = job->next;
        } while (job != NULL);
    }
    // y desbloqueamos la señal
    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);

}

void establece_senales_term(void (*func) (int)) {
    signal(SIGINT, manejador_controlc); // crtl+c interrupt tecleado en el terminal
    signal(SIGQUIT, func); // ctrl+\ quit tecleado en el terminal
    signal(SIGTSTP, func); // crtl+z Stop tecleado en el terminal
    signal(SIGTTIN, func); // proceso en segundo plano quiere leer del terminal
    signal(SIGTTOU, func); // proceso en segundo plano quiere escribir en el terminal
    signal(SIGCHLD, manejador); // manejador sigchld.
}

void init_shell() {
    /* guardar el descriptor de fichero del terminal  */

    shell_terminal = STDIN_FILENO;

    /* ignnorar señales del terminal por parte del shell */
    establece_senales_term(SIG_IGN);

    /* Poner al shell en su propio grupo de procesos  */
    shell_pgid = getpid();
    if (setpgid(shell_pgid, shell_pgid) < 0) {
        perror("EL shell no se pudo poner en su propio process group");
        exit(1);
    }
    /* Coger el terminal  */
    tcsetpgrp(shell_terminal, shell_pgid);

    /* guardar los atributos del terminal para el shell  */
    tcgetattr(shell_terminal, &shell_tmodes);
}

void bg(char **arg,int args) {

    TAREA *job;
    pid_t pid;
    int ntareas,num;

    ntareas = jobs->ntareas;
    // bloqueo señal sigchld...
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);

    if (!ntareas)
    {   // si no hay tareas en ejecución
        printf(":: No encuentro el trabajo. Ejecute jobs para ver la lista de tareas correcta.\n");
        sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);
        return;
    }
    if (args < 2) {
        printf(":: Error en comando. Introduzca help para ayuda.\n");
        sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);
        return;
    }

    job = jobs->inicio;
    pid = job->pgid;

    num = atoi(arg[1]);
    if ((num< 1) || (num >ntareas)) {
        printf(":: No encuentro el trabajo. Ejecute jobs para ver la lista de tareas correcta.\n");
        sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);
        return;
    }
    while (num-->1) job = job->next;
    pid = job->pgid;
    // job es el trabajo a poner el fg
    
    LT_CambiaTareaBg(jobs,pid);
    sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);

    // y le decimos que continúe...
    if (killpg(pid, SIGCONT) == -1) printf(":: Error al reanudar el trabajo.\n");
}

void fg(char **arg,int args) {
    // pone una tarea de segundo plano en primer plano...
    TAREA *job;
    pid_t pid,wpid;
    int num,k,status,ntareas;

    // bloqueo señal sigchld...
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
    // cogemos el primer trabajo de la lista...
    job = jobs->inicio;
    ntareas = jobs->ntareas;
    pid = job->pgid;   // el pid del primer trabajo de la lista...
    

    if (args==1) job->tmodes = shell_tmodes;
        // hay solo un argumento..., asi que ponemos en primer plano la primera tarea de la lista...
    else if (args == 2)
    {
        // hay 2 argumentos, el segundo es el numero de trabajo que queremos poner en primer plano...
        num = atoi(arg[1]);   // num es el numero de trabajo que queremos poner en primer plano...
        if (ntareas<num) {
            // si el numero que doy como argumento es mayor que el numero de tareas de mi lista, no es posible ejecutar la función...
            printf(":: No encuentro el trabajo. Ejecute jobs para ver la lista de tareas correcta.\n");
            sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
            return;
        }
        else while (num-->1) job = job->next;
            // busco el proceso en la lista....
    }
    else
    {
        printf(":: Error en comando. Introduzca help para ayuda.\n");
        sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);
        return;
    }

    pid = job->pgid;
    tcsetpgrp(shell_terminal, pid);
    sigprocmask(SIG_UNBLOCK,&block_sigchld,NULL);
    job->tmodes = shell_tmodes;
    LT_CambiaTareaFg(jobs,pid);
    if (killpg(pid, SIGCONT) == -1) fprintf(stderr, "Error al reanudar el proceso\n");

    // esperar a terminar la tarea de primer plano
    wpid = waitpid(pid, &status, WUNTRACED);

    if (tcsetpgrp(shell_terminal, shell_pgid) == -1) {
        perror("set terminal back to shell");
        exit(0);
    }

    tcsetattr(shell_terminal, TCSANOW, &job->tmodes);

    if (wpid == pid)
        pinta_estado_tarea(wpid, arg[0], status);
    else
        if (wpid == -1) perror("waitpid");

}

int ejecuta_comando(char ** argumentos, int narg) {
    int i, fork_pid, wait_pid, my_pid;
    int status;
    int splano = 0;


    /* si esta vacio */
    if (narg == 0) return 0;
    
    /* comandos internos */
    if (strcmp(argumentos[0], "logout") == 0) return 1;
    if (strcmp(argumentos[0], "help") == 0) {
        help();
        return 0;
    }
    if (strcmp(argumentos[0], "cd") == 0) {
        cd(argumentos, narg);
        return 0;
    }
    if (strcmp(argumentos[0], "prompt") == 0) {
        if (pcomplt == 0) pcomplt++;
        else pcomplt--;
        return 0;
    }
    if (strcmp(argumentos[0], "jobs") == 0) {
        LT_ImprimeTrabajos(jobs, 1);
        return 0;
    }
    if (strcmp(argumentos[0], "fg") == 0) {
        fg(argumentos,narg);
        return 0;
    }
    if (strcmp(argumentos[0], "bg") == 0) {
        bg(argumentos,narg);
        return 0;
    }
    if (strcmp(argumentos[0], "cls") == 0) {
        system("clear");
        return 0;
    }
    if (strcmp(argumentos[0], "ver") == 0) {
        printf("Shell3B 2010 versión %s\n",VER);
        return 0;
    }
    /* Comprobamos si se está solicitando la ejecución en segundo plano... */
    /* Buscamos ampersand */
    if (strcmp(argumentos[narg - 1], "&") == 0) {
        splano = 1;
        narg--;
        free(argumentos[narg]);
        argumentos[narg] = NULL;
    }
    /* ejecuta comando externo */
    /* lanza la ejecucion de un proceso hijo */

    fork_pid = fork();
    // bloquear sigchld ...
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);

    
    // añadimos el proceso a la lista de procesos como de primer plano
    LT_AnadeTrabajo(jobs, fork_pid, argumentos[0], 2, shell_tmodes);
    if (splano) {
        LT_CambiaTareaSegPlano(fork_pid, jobs);
        if (fork_pid>0) printf(":: Ejecutando trabajo %s [pid:%d] en segundo plano.\n",argumentos[0],fork_pid);
    }


    // vuelvo a desbloquear sigchld.
    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);


    if (fork_pid == 0) { /* hijo */
        // recupero el pid.
        my_pid = getpid();
        setpgid(my_pid, my_pid); // poner al proceso en su propio grupo de procesos
        if (!splano)
            tcsetpgrp(shell_terminal, my_pid); // asignarle el terminal si es de fg

        /* devolver comportamiento de señales a default  */
        establece_senales_term(SIG_DFL);
        execvp(argumentos[0], argumentos); // ejecutar el código del comando

        printf(":: Error, no se puede ejecutar: %s\n", argumentos[0]);
        perror("execcvp");
        exit(-1);
    } else if (fork_pid < 0) {
        /* fork no se realizón con exito */
        perror("fork");
        exit(1);
    }
    /* padre */

    setpgid(fork_pid, fork_pid);
    /* tarea de primer plano ....*/
    if (!splano) {

        tcsetpgrp(shell_terminal, fork_pid); // asignarle el terminal

        // esperar a terminar la tarea de primer plano
        wait_pid = waitpid(fork_pid, &status, WUNTRACED);

        /* volver a poner el shell en primer plano (capturar el terminal) */
        if (tcsetpgrp(shell_terminal, shell_pgid) == -1) {
            perror("set terminal back to shell");
            exit(0);
        }

        /* restaurar los atributos del terminal para el shell (la tarea puede haberlos 	cambiado) */
        tcsetattr(shell_terminal, TCSANOW, &shell_tmodes);

        // pintar estado de terminacion de la tarea

        if (wait_pid == fork_pid)
            pinta_estado_tarea(wait_pid, argumentos[0], status);
        else
            if (wait_pid == -1) perror("waitpid");

    }

    return 0;
}

int main(int argc, char *argv[], char *env[]) {
    char *argumentos[MAXARG];
    int narg, i, j, status;
    int fin = 0;
    char linea[MAXLINEA];

    // Inicializa la lista de tareas.
    jobs = (TAREAS *) malloc(sizeof (TAREAS));
    LT_Inicializa(jobs);
    // Inicializa la máscara para SIGCHLD
    sigemptyset(&block_sigchld);
    sigaddset(&block_sigchld, SIGCHLD);
    // Inicializa el shell
    init_shell();
    // Bienvenida y ayuda ppal.
    intro();

    while (!fin) {
        /* escribe el prompt */
        printf("%s", PROMPT);
        if (pcomplt) printf(":%s>", (char *) get_current_dir_name());
        else printf(">");
        /* lee la linea de comandos*/
        nueva_linea(linea, MAXLINEA);
        /* analiza linea de comandos y la separa en argumentos */
        narg = lee_linea(linea, argumentos);
        /* ejecuta linea comandos */
        fin = ejecuta_comando(argumentos, narg);
        /* libera memoria de los argumentos */
        libera_mem_arg(argumentos, narg);
    }
    // Finaliza lista y libera memoria...
    LT_BorrarTareas(jobs);
    printf("Adios.\n");

    exit(0);
}
