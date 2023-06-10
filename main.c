#include <stddef.h> /* NULL */
#include <stdio.h>	/* setbuf, printf */
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <pwd.h>
#include <glob.h>

// Funcion de reseteo de descriptores originales del shell
void resetOriginal(int fdOriginal[3])
{
	dup2(fdOriginal[0], 0);
	dup2(fdOriginal[1], 1);
	dup2(fdOriginal[2], 2);
}

// funcion de redirecciones de entrada, salida y salida de error
int redireccion(char *filev[3])
{
	int fdRedirecciones[3];
	if (filev[2]) // si hay redireccion de error
	{
		fdRedirecciones[2] = creat(filev[2], 0666); // obtengo el fd necesario
		if (fdRedirecciones[2] < 0)					// caso error
		{
			perror("open"); // transforma errno en un mensaje de error
			return 1;
		}
		else
		{ // reemplazo los fd correspondientes con el obtenido anteriormente y se cierra ya que no se va a volver a utilizar
			dup2(fdRedirecciones[2], 2);
			close(fdRedirecciones[2]);
		}
	}

	// Redireccion de salida estandar
	if (filev[1]) // si hay redireccion de salida
	{
		fdRedirecciones[1] = creat(filev[1], 0666); // obtengo el fd necesario
		if (fdRedirecciones[1] < 0)					// caso error
		{
			perror("open"); // transforma errno en un mensaje de error
			return 1;
		}
		else
		{ // reemplazo los fd correspondientes con el obtenido anteriormente y se cierra ya que no se va a volver a utilizar
			dup2(fdRedirecciones[1], 1);
			close(fdRedirecciones[1]);
		}
	}

	// Redireccion de entrada estandar
	if (filev[0]) // si hay redireccion de entrada
	{
		fdRedirecciones[0] = open(filev[0], O_RDONLY); // obtengo el fd necesario
		if (fdRedirecciones[0] < 0)					   // caso error
		{
			perror("open"); // transforma errno en un mensaje de error
			return 1;
		}
		{ // reemplazo los fd correspondientes con el obtenido anteriormente y se cierra ya que no se va a volver a utilizar
			dup2(fdRedirecciones[0], 0);
			close(fdRedirecciones[0]);
		}
	}
	return 0;
}

// Funcion para comprobar si es un mandato interno
int esInterno(char *comando)
{
	if (strcmp(comando, "cd") == 0 || strcmp(comando, "umask") == 0 || strcmp(comando, "time") == 0 || strcmp(comando, "read") == 0)
	{ // se compara el string con los posibles comandos internos tratados
		return 1;
	}

	return 0;
}

// Funcion de ejecucion de mandatos internos
int ejecutarInterno(char **argv)
{
	int i;
	for (i = 0; argv[i + 1] != NULL; i++)
	{
	} // se cuenta el numero de argumentos sin contar el argumento del comando interno
	int argc = i;

	// Ejecucion del mandato interno cd
	if (strcmp(argv[0], "cd") == 0)
	{
		if (argc < 2)
		{
			if (argv[1] == NULL)
			{									// caso en el que se escriba cd sin ningun directorio al que cambiar
				if (chdir(getenv("HOME")) != 0) // se obtiene la ruta de HOME y se cambia de directorio a HOME
				{								// caso de error
					fprintf(stderr, "error en la ejecicion del comando cd\n");
					return 1;
				}
				else
				{
					char cwd[256]; // buffer para la ruta
					if (getcwd(cwd, sizeof(cwd)) == NULL)
					{ // se obtiene la direccion del directorio actual de trabajo
						perror("getcwd() error");
						return 1;
					}
					else
					{ // se imprime la direccion del directorio actual de trabajo
						printf("%s\n", cwd);
					}
				}
			}
			else
			{ // caso en el que se escriba cd sin ningun directorio al que cambiar
				if (chdir(argv[1]) != 0)
				{ // se intenta cambiar al directorio
					fprintf(stderr, "error en la ejecicion del comando cd\n");
					return 1;
				}
				else
				{ // obtencion del working directory e impresion del mismo
					char cwd[256];
					if (getcwd(cwd, sizeof(cwd)) == NULL)
					{
						perror("getcwd() error");
						return 1;
					}
					else
					{
						printf("%s\n", cwd);
					}
				}
			}
		}
		else
		{ // en el caso de que haya un numero incorrecto de argumentos se imprime un error
			fprintf(stderr, "numero incorrecto de argumentos para el comando cd\n");
			return 1;
		}
	}
	// Ejecucion del mandato interno umask
	else if (strcmp(argv[0], "umask") == 0)
	{
		if (argc < 2)
		{
			if (argv[1] == NULL)
			{
				mode_t mask = umask(0); // obtener la máscara actual y establecerla en 0
				umask(mask);			// restaurar la máscara original
				printf("%o\n", mask);	// imprimir la mascara en formato octal
			}
			else
			{
				mode_t old_mask = umask(0);					  // obtener la mascara antigua
				char *endptr;								  // el primer caracter invalido al usar strtol se almacena aqui
				long int octal = strtol(argv[1], &endptr, 8); // se obtiene el valor de la mascara
				if (*endptr != '\0' || octal < 0 || octal > 0777)
				{ // caso mascara no valida
					fprintf(stderr, "umask: valor octal inválido\n");
					umask(old_mask);
					return 1;
				}
				else
				{ // en caso de obtencion de la mascara exitosa se sustituye y se imprime la mascara antigua
					umask(octal);
					printf("%o\n", old_mask);
				}
			}
		}
		else
		{
			fprintf(stderr, "numero incorrecto de argumentos para el comando umask\n");
			return 1;
		}
	}
	// Ejecucion del mandato interno time
	else if (strcmp(argv[0], "time") == 0)
	{
		struct tms tiempo_inicio, tiempo_fin; // structs para almacenar el tiempo de inicio y de final
		clock_t inicio, fin;				  // tiempos de inicio y fin
		if (argc == 0)
		{
			// se obtienen los tiempos de inicio y de fin
			inicio = times(&tiempo_inicio);
			fin = times(&tiempo_fin);
			// Imprimir los tiempos
			printf("%d.%03du %d.%03ds %d.%03dr\n",
				   (int)((tiempo_fin.tms_utime - tiempo_inicio.tms_utime) / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_utime - tiempo_inicio.tms_utime) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_stime - tiempo_inicio.tms_stime) / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_stime - tiempo_inicio.tms_stime) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)),
				   (int)((fin - inicio) / sysconf(_SC_CLK_TCK)),
				   (int)((fin - inicio) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)));
		}
		else
		{
			inicio = times(&tiempo_inicio); // se obtienen los tiempos de inicio

			int pid = fork();
			if (pid > 0)
			{
				while (pid != wait(NULL))
					;
			}
			else
			{
				if (esInterno(argv[1]))
				{									 // ejecucion de mandatos internos
					exit(ejecutarInterno(&argv[1])); // ejecucion mandato interno
				}
				else
				{
					execvp(argv[1], &argv[1]); // ejecucion del mandato
					exit(1);
				}
			}
			fin = times(&tiempo_fin); // se obtienen los tiempos de fin
			// Imprimir los tiempos
			printf("%d.%03du %d.%03ds %d.%03dr\n",
				   (int)((tiempo_fin.tms_utime - tiempo_inicio.tms_utime) / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_utime - tiempo_inicio.tms_utime) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_stime - tiempo_inicio.tms_stime) / sysconf(_SC_CLK_TCK)),
				   (int)((tiempo_fin.tms_stime - tiempo_inicio.tms_stime) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)),
				   (int)((fin - inicio) / sysconf(_SC_CLK_TCK)),
				   (int)((fin - inicio) % sysconf(_SC_CLK_TCK) * 1000 / sysconf(_SC_CLK_TCK)));
		}
	}
	// Ejecucion del mandato interno read
	else if (strcmp(argv[0], "read") == 0)
	{
		char line[2049];		  // Buffer para almacenar la línea de entrada
		fgets(line, 2048, stdin); // Leer una línea de la entrada estándar
		int i = 0;

		char *token = strtok(line, " \t"); // Obtener la primera palabra utilizando strtok
		while (token != NULL && argv[i + 1] != NULL)
		{ // Repetir hasta que no haya más palabras

			char *colocar = malloc(strlen(argv[i + 1]) + strlen(token) + 1); // crear string con tamaño suficiente
			sprintf(colocar, "%s=%s", argv[i + 1], token);					 // obtener el string con el formato necesario para el putenv
			colocar[strlen(colocar)] = '\0';								 // poner el caracter nulo al final del string
			putenv(colocar);												 // colocar la variable de entorno

			i++;
			if (argv[i + 2] == NULL)
			{
				token = strtok(NULL, "\n"); // obtener la ultima palabra utilizando strtok (evitar que coja el salto de linea)
			}
			else
			{
				token = strtok(NULL, " \t\n"); // Obtener la siguiente palabra utilizando strtok
			}
		}
	}
	return 0;
}

// sustituciones de metacaracteres
void sustituciones(char ***argvv)
{
	int j = 0;
	int i = 0;
	while (argvv[i] != NULL)
	{
		while (argvv[i][j] != NULL) // recorrer todos los elementos de la matriz
		{

			char *palabra = NULL;
			palabra = argvv[i][j];
			if (palabra[0] == '~')
			{												 // CASO ~
				if (palabra[1] == '/' || palabra[1] == '\0') // caso tilde seguida de / o \0 directamente
				{
					char *home = getenv("HOME"); // se obtiene la ruta al directorio HOME
					if (palabra[1] == '\0')
					{																			 // caso de que la tilde este sola
						argvv[i][j] = (char *)realloc(argvv[i][j], sizeof(char) * strlen(home)); // se crea espacio suficiente para la direccion
						strcpy(argvv[i][j], home);												 // se copia la direccion
					}
					else
					{
						char *res = NULL;
						res = (char *)malloc(sizeof(char) * (strlen(home) + strlen(palabra) + 1)); // se crea espacio suficiente para el strin resultante
						strncpy(res, home, strlen(home) + 1);									   // se copia la direccion de HOME
						strcat(res, &palabra[1]);												   // se concatena el resto del string
						argvv[i][j] = (char *)realloc(argvv[i][j], sizeof(char) * strlen(res));	   // se crea espacio suficiente para el resultado
						strcpy(argvv[i][j], res);												   // se copia el resultado
						free(res);																   // se libera la memoria
					}
				}
				else if ((palabra[1] >= 'a' && palabra[1] <= 'z') || (palabra[1] >= 'A' && palabra[1] <= 'Z') || (palabra[1] >= '0' && palabra[1] <= '9') || palabra[1] == '_')
				{
					// Si ~ está seguido de un usuario con formato válido
					char usuario[strlen(palabra) - 1];			   // se crea un string para almacenar el usuario
					sscanf(palabra + 1, "%[_a-zA-Z0-9]", usuario); // se obtiene el usuario con formato valido
					struct passwd *pw = NULL;					   // se crea el struct necesario
					pw = getpwnam(usuario);						   // se obtiene el struct correspondiente al usuario
					char *home = pw->pw_dir;					   // se obtiene la direccion HOME del usuario en cuestion

					char *res = NULL;
					res = (char *)malloc(sizeof(char) * (strlen(home) + strlen(palabra) - 1 - strlen(usuario) + 1)); // se crea espacio suficiente para el string resultante
					strncpy(res, home, strlen(home) + 1);															 // se copia la direccion del HOME del usuario
					strcat(res, &palabra[1 + strlen(usuario)]);														 // se concatena el resto del string inicial

					argvv[i][j] = (char *)realloc(argvv[i][j], sizeof(char) * (strlen(home) + strlen(palabra) - 1 - strlen(usuario))); // se crea el espacio suficiente para el string resultado
					strcpy(argvv[i][j], res);																						   // se copia el string resultante
					free(res);																										   // se libera la memoria
				}
			}
			char *tmp = argvv[i][j];
			while ((tmp = strchr(tmp, '$')) != NULL) // caso del $
			{
				char *variable = NULL;
				if (sscanf(tmp + 1, "%m[_a-zA-Z0-9]", &variable) > 0) // se obtiene el nombre de la variable
				{													  // si el nombre de la variable es valido
					char *valor = NULL;
					valor = getenv(variable); // obtengo el valor de la variable
					if (valor != NULL)
					{						 // si el valor estaba establecido anteriormente (existe la variable)
						int start_index = 0; // indice  donde se encuentra el $
						char *res = NULL;
						start_index = tmp - argvv[i][j];													   // se establece el indice
						res = malloc(sizeof(char) * (strlen(argvv[i][j]) + strlen(valor) - strlen(variable))); // se crea espacio suficiente para almacenar el string resultante
						strncpy(res, argvv[i][j], start_index);												   // copio lo anterior al $ sin poner el caracter nulo
						strcpy(res + start_index, valor);													   // copio el valor de la variable
						strcat(res, tmp + strlen(variable) + 1);											   // Copiamos la parte de la palabra posterior al fragmento a sustituir
						argvv[i][j] = (char *)realloc(argvv[i][j], sizeof(char) * (strlen(res) + 1));
						strcpy(argvv[i][j], res);						 // se copia el resultado en la matriz
						free(res);										 // se libera la memoria
						tmp = argvv[i][j] + start_index + strlen(valor); // Avanzamos el puntero temporal para que no se analice la parte de la palabra ya sustituida
					}
					else
					{
						tmp += strlen(variable) + 1; // en el caso de que la variable no existiese se actualiza el lugar desde donde
													 // se tiene que empezar a buscar un nuevo $
					}
				}
				else
				{
					tmp++; // en el caso de que no haya nada despues del $  se actualiza el lugar desde donde se tiene que empezar
						   // a buscar un nuevo $
				}
			}
			char *posicion = argvv[i][j];
			if (strchr(posicion, '?') != NULL && strchr(posicion, '/') == NULL) //expansion de ?
			{
				glob_t resultado; // struct necesario
				char patron[strlen(posicion) + 1]; // string para albergar temporalmente la palabra
				strcpy(patron, posicion); // // copia la palabra en el string
				memset(&resultado, 0, sizeof(resultado)); // se crea espacio para almacenar el struct
				glob(patron, 0, NULL, &resultado); // se obtiene el struct
				if (resultado.gl_pathc == 1)
				{ // caso de un unico match con el patron
					free(argvv[i][j]); // se libera la memoria
					argvv[i][j] = strdup(resultado.gl_pathv[0]); // se almacena el nuevo string
				}
				else if (resultado.gl_pathc > 1)
				{ // caso de varios matches con el patron
					int n = 0;
					while (argvv[i][n] != NULL)
					{ // se obtiene el numero total de argumentos
						n++;
					}
					char **argv = (char **)malloc((resultado.gl_pathc + n) * sizeof(char *)); // creo hueco suficiente para almacenar todos los nuevos argumentos
					int k = 0;
					while (argvv[i][k] != NULL)
					{
						if (k < j)
						{
							argv[k] = strdup(argvv[i][k]);
						}
						else if (k == j)
						{
							int l = 0;
							for (; l < resultado.gl_pathc; l++)
							{
								argv[k + l] = strdup(resultado.gl_pathv[l]);
							}
						}
						else
						{
							argv[k + resultado.gl_pathc - 1] = strdup(argvv[i][k]);
						}
						k++;
					}
					argv[k + resultado.gl_pathc - 1] = NULL; // añado el elemento nulo
					int u = 0;
					while (argvv[i][u] != NULL)
					{
						free(argvv[i][u]);
						u++;
					}
					argvv[i] = (char **)realloc(argvv[i], sizeof(char *) * (resultado.gl_pathc + n));
					argvv[i] = argv;
				}
				globfree(&resultado);
			}

			j++;
		}
		j = 0;
		i++;
	}
}

extern int obtain_order(); /* See parser.y for description */

int main(void)
{
	char ***argvv = NULL;
	int argvc;
	// char **argv = NULL;
	//  int argc;
	char *filev[3] = {NULL, NULL, NULL};
	int bg;
	int ret;

	setbuf(stdout, NULL); /* Unbuffered */
	setbuf(stdin, NULL);

	int fdOriginal[3] = {dup(STDIN_FILENO), dup(STDOUT_FILENO), dup(STDERR_FILENO)};
	int bgpid;

	signal(SIGQUIT, SIG_IGN);
	signal(SIGINT, SIG_IGN);

	// variables especiales MYPID
	char colocarMYPID[100];
	sprintf(colocarMYPID, "mypid=%d", getpid());
	putenv(colocarMYPID);

	// variables especiales PROMPT
	putenv("prompt=msh> ");

	// variables especiales STATUS
	int status;

	while (1)
	{
		while (waitpid(-1, NULL, WNOHANG) > 0)
			; // bucle para eliminar procesos zombie

		// actualizacion de la variable especia PROMPT
		fprintf(stderr, "%s", getenv("prompt")); /* Prompt */

		ret = obtain_order(&argvv, filev, &bg);
		if (ret == 0)
			break; /* EOF */
		if (ret == -1)
			continue;	 /* Syntax error */
		argvc = ret - 1; /* Line */
		if (argvc == 0)
			continue; /* Empty line */

		int pid;
		int fd[2];
		sustituciones(argvv);

		// Se comprueba que no haya errores en las redirecciones
		if (redireccion(filev) == 0)
		{
			char colocarSTATUS[100];
			// Bucle para metodo iterativo
			for (int i = 0; i < argvc; i++)
			{
				if (i < argvc - 1)
				{ // Creacion de pipe necesario
					pipe(fd);
				}
				if (bg || !(i == argvc - 1 && esInterno(argvv[argvc - 1][0])))
				{ // Creacion de hijos en caso de que el ultimo mandato no sea interno o sea un mandato a realizar en background
					pid = fork();
				}
				if (argvc == 1 && esInterno(argvv[argvc - 1][0]) && !bg)
				{ // Ejecucion del mandato interno ultimo si no esta activo el background
					sprintf(colocarSTATUS, "status=%d", ejecutarInterno(argvv[i]));
					putenv(colocarSTATUS);
				}
				else if (pid > 0)
				{ // proceso padre
					if (i < argvc - 1)
					{ // Conexiones en las tuberias
						dup2(fd[0], 0);
						close(fd[0]);
						close(fd[1]);
					}
					if (!bg && i == argvc - 1 && esInterno(argvv[i][0]))
					{ // Ejecucion del mandato interno ultimo si no esta activo el background
						ejecutarInterno(argvv[i]);
					}
				}
				else
				{ // proceso hijo
					if (!bg)
					{ // Comportamiento por defecto para las señales SIGQUIT y SIGINT
						signal(SIGQUIT, SIG_DFL);
						signal(SIGINT, SIG_DFL);
					}
					if (i < argvc - 1)
					{ // conexiones de tuberias para todos menos el ultimo
						dup2(fd[1], 1);
						close(fd[0]);
						close(fd[1]);
					}
					if (esInterno(argvv[i][0]))
					{ // ejecucion de mandatos internos
						exit(ejecutarInterno(argvv[i]));
					}
					else
					{ // ejecucion de mandato no interno
						execvp(argvv[i][0], argvv[i]);
						exit(1);
					}
				}
			}
			if (!(argvc == 1 && esInterno(argvv[argvc - 1][0]) && !bg))
			{ // cuando no sea un unico comando interno a ejecutar en el shell padre
				if (!bg)
				{ // caso mandato no en background
					while (pid != wait(&status))
						; // bucle de espera al ultimo hijo

					sprintf(colocarSTATUS, "status=%d", status);
					putenv(colocarSTATUS);
				}
				else
				{ // caso de mandato en background
					bgpid = pid;
					fprintf(stderr, "[%d]\n", bgpid);

					// variables especiales
					char colocarBG[100];
					sprintf(colocarBG, "bgpid=%d", bgpid);
					putenv(colocarBG);
				}
			}
		}

		resetOriginal(fdOriginal); // reseteo de descriptores estandar
	}
	exit(0);
	return 0;
}
