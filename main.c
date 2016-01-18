// Julián Toledano Díaz

// Práctica Sistemas Operativos

/******************************************************
* La acción -pipe no está implementada. Así como los  *
* errores 2, 3, 4, 5.                                 *
*******************************************************/

#include <stdio.h>
#include <string.h> // strcmp(), strstr()
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // O_RDONLY

// Devuelve el tamaño de la lista tras aplicar un filtro
// path: dirección donde realizar la lista
// type: en que se basa la seleccion. Posibles valores -n -t -p -c -C
// list structura del tipo dirent donde se guardarán todos los dir que cumplan el filtro
int selection(char *path, char *type, struct dirent ***list);

// Filtro para scandir de subcadenas, utilizado para búsqueda basada en nombre de archivo.
// Devuelve uno si exite la subcadena y 0 si no.
// Utiliza la variable global global_selection_parameter.
int nameFilter(const struct dirent *entry);

// Filtro para scandir basada en el tipo de archivo, directorios a archivos regulares.
// Utiliza la variable global global_selection_parameter.
/*********************************************************************************************
* BUG: si se ejecuta la misma accion varias veces unicamente encontrará los directorios . .. *
* (trabajando en Debian GNU/Linux version 7.7)                                               *
**********************************************************************************************/
int fileTypeFilter(const struct dirent *entry);

// Filtro para scandir basado en los permisos del usuario que está ejecutando el proceso.
// 1º comprueba que el usuario es el mismo que el propietario del archivo
// 2º comprueba si pertenecen al mismo grupo
// 3º Si no aplica el valor de usuario "ajeno"
// Utiliza la variable global global_selection_parameter.
int permissionsFilter(const struct dirent *entry);

// Filtro para scandir que comprueba si existe una subcadena dentro de un archivo.
// Precondicion: no puede ser un directorio. Debe ser un regular file.
// Utiliza la variable global global_path.
int contentFilter(const struct dirent *entry);


// Se vierte toda la información en un bloque de memoria que posteriormente se comprueba con memmem
// para ver si existe la string argv[]
int contentFilterMemmory(const struct dirent *entry);


// Imprime la lista list
void printEntries(struct dirent **list, int size);

// Se crea un proceso nuevo para realizar la acción argv[5], con parámetros argv[6]
// sobre cada uno de los directorios y archivos de la selección.
// Un proceso no comienza hasta que no acaba el anterior
void secuentialExecution(struct dirent **list, int size, char *mandate, char *argumentos);


// Vacia la memoria
// Precondición size debe de ser mayor que cero
void freeEntries(struct dirent **list, int size);


// Variable global necesaria para pasar el argumento argv[3](parámetros del tipo de selección) a la función filter de scandir
// int (*filter)(const struct dirent *)
static const char *global_selection_parameter;

// Variable global necesaria para pasar el argumento argv[1](path) al filtro por contenido.
static const char *global_path;



int main(int argc, char* argv[]){



    struct dirent **directoryList;
    int n;

    char *path = argv[1]; //argv[1]. Directoria de busqueda
    global_path = path;
    char *tipeSelection = argv[2]; // argv[2]. Posibles valores: -n, -t -p -c -C d
    global_selection_parameter = argv[3]; //argv[3]. Posibles valores: cadena, d, f, x, w, r
    char *action = argv[4]; // argv[4]. Posibles valores: -print -pipe, -exec
    char *mandate = argv[5]; // Acción a ejecutar
    char *argumentos = argv[6];



    if((strcmp(tipeSelection,"-n")!=0)&&(strcmp(tipeSelection,"-t")!=0)&&(strcmp(tipeSelection,"-p")!=0)&&(strcmp(tipeSelection,"-c")!=0)&&(strcmp(tipeSelection,"-C")!=0)&&(strcmp(tipeSelection,"-d")!=0)){
       printf("Error(0): Opcion no valida\n"); return 0;
    }
    n = selection(path, tipeSelection, &directoryList);//argv[1]

    // Imprimimos la lista
    if(strcmp(action, "-print") == 0)
        printEntries(directoryList,n);
    // Ejecutar programa por archivo
    else if(strcmp(action,"-exec") == 0)
        secuentialExecution(directoryList, n, mandate, argumentos);

    // Liberamos la memoria
    freeEntries(directoryList,n);

    return 0;
}

/*************************
* FUNCIONES DE SELECCION *
**************************/


int selection(char *path, char *type, struct dirent ***list){
     // Seleccion por nombre.
    if(strcmp(type, "-n") == 0)
        return scandir(path, list, nameFilter, alphasort);

    // Seleccion por tipo de archivo.
    else if(strcmp(type, "-t") == 0){
        if((strcmp(global_selection_parameter,"d") !=0)&&(strcmp(global_selection_parameter,"-")!=0)){
           printf("Error(0): Opcion no valida\n"); return 0;
        }else
            return scandir(path,list,fileTypeFilter,alphasort);
    }

   // Selección por permisos.
   else if(strcmp(type,"-p") == 0){
        if((strcmp(global_selection_parameter,"r") !=0)&&(strcmp(global_selection_parameter,"w")!=0)&&(strcmp(global_selection_parameter,"x")!=0)){
           printf("Error(0): Opcion no valida\n"); return 0;
        }else
            return scandir(path,list,permissionsFilter,alphasort);
   }

    // Seleccion por contenido.
    else if(strcmp(type,"-c") == 0)
        return scandir(path,list,contentFilter,alphasort);

    // Selección por contenido vertido en memoria.
    else if(strcmp(type,"-C") == 0)
        return scandir(path,list,contentFilterMemmory,alphasort);

   return 0;
}

int nameFilter(const struct dirent *entry){
    // Comprueba que la subcadena existe dentro del nombre de la entrada.
    if(strstr(entry->d_name, global_selection_parameter) != NULL)
       return 1;
    return 0;
}

int fileTypeFilter(const struct dirent *entry){
    struct stat st;
    stat(entry->d_name, &st);
    if(strcmp(global_selection_parameter, "d") == 0)
        return (st.st_mode & S_IFDIR);
    else if(strcmp(global_selection_parameter, "f") == 0)
        return (st.st_mode & S_IFREG);
    return 0;
    }

int permissionsFilter(const struct dirent *entry){
    struct stat st;
    stat(entry->d_name, &st);

    // Si el uid del usuario que está ejecutando la aplicación y el uid del archivo son iguales.
    if(getuid() == st.st_uid){
        // Además el usuario introdujo r
        if(strcmp(global_selection_parameter, "r") == 0)
            return (st.st_mode & S_IRUSR);
        // Además el usuario introdujo w
        else if(strcmp(global_selection_parameter, "w") == 0)
            return (st.st_mode & S_IWUSR);
        // Además el usuario introdujo x
        else if(strcmp(global_selection_parameter, "x") == 0)
            return (st.st_mode & S_IXUSR);
    }
    // Si el gid del usuario coincide con el gid del archivo.
     else if(getgid() == st.st_gid){
        // Además el usuario introdujo r
        if(strcmp(global_selection_parameter, "r") == 0)
            return (st.st_mode & S_IRGRP);
        // Además el usuario introdujo w
        else if(strcmp(global_selection_parameter, "w") == 0)
            return (st.st_mode & S_IWGRP);
        // Además el usuario introdujo x
        else if(strcmp(global_selection_parameter, "x") == 0)
            return (st.st_mode & S_IXGRP);
    }
    // Si ni el uid ni el gid del usuario coinciden con el uid y el gdi de la aplicación.
    else{
        // Además el usuario introdujo r
        if(strcmp(global_selection_parameter, "r") == 0)
            return (st.st_mode & S_IROTH);
        // Además el usuario introdujo w
        else if(strcmp(global_selection_parameter, "w") == 0)
            return (st.st_mode & S_IWOTH);
        // Además el usuario introdujo x
        else if(strcmp(global_selection_parameter, "x") == 0)
            return (st.st_mode & S_IXOTH);
    }

    return 0;
}

int contentFilter(const struct dirent *entry){
    struct stat st;
    stat(entry->d_name,&st);

    // Obtenemos el path
    char path [256]; // Cuidado solo se pueden guardar hasta 256 caractéres.
    strncpy(path, global_path, sizeof(path));
    strncat(path, "/", sizeof(path));
    strncat(path, entry->d_name, sizeof(path));

    // Comprobar que es un fichero.
    if(st.st_mode & S_IFREG){
        // Abrimos el fichero
        FILE *file;
        file = fopen(path,"r"); // Abrimos solo en modo lectura.
        int encontrado = 0; // flag false
        char tmp[256]={0x0};

        while(file != NULL && fgets(tmp, sizeof(tmp), file) !=NULL && !encontrado)
                if (strstr(tmp, global_selection_parameter) != NULL) encontrado = 1;

        // Cerramos el fichero.
        if(file != NULL) fclose(file);
        if(encontrado) return 1;
    }
    return 0;
}

int contentFilterMemmory(const struct dirent *entry){
    struct stat st;
    stat(entry->d_name,&st);

    char path[256]; // Cuidado solo se pueden guardar hasta 256 caractéres.
    strncpy(path, global_path, sizeof(path));
    strncat(path, "/", sizeof(path));
    strncat(path, entry->d_name, sizeof(path));

    // Comprobamos que es un fichero
    if(st.st_mode & S_IFREG){
        const char *memoryBlock;
        int fd;
        struct stat sb;
        fd = open(path, O_RDONLY);
        fstat(fd, &sb);
        memoryBlock = mmap(NULL, sb.st_size,PROT_READ,MAP_SHARED,fd,0); // Proyectamos el archivo a una zona de memoria elejida por el sistema operativo (fallará si el archivo es muy grande).
        if(memoryBlock == MAP_FAILED) printf("ERROR");
        char *p =(char*) memmem(memoryBlock, strlen(memoryBlock), global_selection_parameter, strlen(global_selection_parameter)); // Devuelve un puntero al comienzo de la substring si existe, NULL si no existe.
        if(p!= NULL) {
            close(fd);
            return 1;
        }
        close(fd);
    }
    return 0;
}


/************************
* FUNCIONES DE ACCION   *
*************************/

void printEntries(struct dirent **list, int size){
    int i;
    for(i = 0; i < size; i++)
        printf("%s\n", list[i]->d_name);

}

void secuentialExecution(struct dirent **list, int size, char *mandate, char *argumentos){
    int i;
    pid_t pid;
    for (i=0; i<size; i++)
     {
        char pathTest[256];
        strncpy(pathTest, global_path, sizeof(pathTest));
        // Comprobar que no se introduce el directorio raiz como argumento argv[1] o accederemos al directorio // en vez de a los correspondientes directorios de la raiz (/).
        if((strcmp(global_path, "/") != 0))
            strncat(pathTest, "/", sizeof(pathTest));
        strncat(pathTest, list[i]->d_name, sizeof(pathTest));

        char *name[] = {
            mandate,
            argumentos,
            pathTest,
            NULL
            };

        pid = fork();
         if (pid == -1)
         {
             perror("Error al crear el proceso.");
             return -1;
         }
         else if (pid > 0) // Proceso padre
         {
             // waitpid suspende la ejecución hasta el cambio de estado de un proceso hijo,
             // en este caso -1, esperara a cualquier proceso hijo.
             waitpid(-1, NULL, 0);
         }
         else
         {
            // Reemplaza el proceso por uno nuevo en este caso la ejecución de /bin/bash
             execvp(name[0], name);
             exit(0);
         }
     }
}

void freeEntries(struct dirent **list, int size){
    if (size <= 0) return;
    int i;
    for (i = 0; i < size; i++)
        free(list[i]);
    free(list);
}


