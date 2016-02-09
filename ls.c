#define _XOPEN_SOURCE //Se necesita para que sys/stat declare algunas constantes necesarias
#include <unistd.h>
#include <stdio.h>
#include <dirent.h> //Contiene la estructura dirent
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

//Este arreglo contiene los atributos a ser ejecutados
int commands[ 11 ];

/*
!0: -A : lists all entries including those starting with periods (.), but excluding any . or .. entries.
!1: -h : displays file sizes using more human-friendly units.
!2: -i : displays inode numbers along with file names (only on systems that support inode numbers, such as POSIX-compliant and UNIX systems).
!3: -l : displays permissions, links, owner, group, size, time, name; see Long Output Format.
!4: -n : displays user ID and group IDs as numbers instead of names.
!5: -o : displays only the user ID of owner (POSIX-compliant and UNIX systems only).
!6: -p : puts / after directory names.
!7: -a : lists all entries including those starting with periods (.)
*/

char units[ 7 ] = {
	'B',
	'K',
	'M',
	'G',
	'T',
	'P',
	'E'
};

//Funciones auxiliares
char *file_size_human( int ); //Convierte un tamano de archivo a formato facil de entender
char *mode_human( int ); //Representa los permisos de los archivos en formato facil de entender
char *date_human( time_t fecha ); //Convierte la fecha de entero a string

int main( int argc, char * argv[ ] ) {
	int ch;

	//Marco todos los comandos como deshabilitados al principio
	for( int i = 0; i < 11; i++ ){
		commands[ i ] = 0;
	}

	//Si es root implementar -A automaticamente
	if ( !getuid( ) ){
		commands[ 0 ] = 1;
	}

	//Con getopt puedo procesar los parametros de tipo - y --, muy facilmente
	while ( ( ch = getopt(argc, argv, "1ARahilnop" ) ) != -1 ) {
		switch( ch ){
			case 'A': //Con este argumento se listan los archivos ocultos
				commands[ 0 ] = 1;
			break;

			case 'h': //Mostrar los tamanos de archivos en formato humano/entendible
				commands[ 1 ] = 1;
			break;

			case 'i': //Mostrar el nodo i del elemento
				commands[ 2 ] = 1;
			break;

			case 'l': //Mostrar informacion larga/detallada
				commands[ 3 ] = 1;
			break;

			case 'n': //Con esto muestro solo los ids en vez de los nombres en owner y group
				commands[ 4 ] = 1;
			break;

			case 'o': //Solo mostrar solo id del usuario owner
				commands[ 5 ] = 1;
			break;

			case 'p': //Si el elemento es un directorio se le agrega '/' al final
				commands[ 6 ] = 1;
			break;

			case 'a': //Muestro las entradas . y ..
				commands[ 7 ] = 1;
			break;
		}
	}

	//Directorio donde me encuentro
	char Directory[ 1024 ];
	DIR *dir; //Estructura directorio que representa un flujo de directorio
	struct dirent *ent; //Esta estructura contiene el nombre del directorio/fichero

	//Con esta funcion me sitio en la carpeta desde donde estoy llamando al script
	if ( getcwd( Directory, sizeof( Directory ) ) == NULL ){
		printf( "Error obteniendo el directorio base..." );
		return 0;
	}

	//Abro el directorio actual
	if ( ( dir = opendir ( Directory ) ) != NULL) {
		//Mientras tenga archivos
		while ( ( ent = readdir ( dir ) ) != NULL ) {
			struct stat st; //Esta es la estructura que contiene la informacion del elemento

			//Si no esta definido el comando -a, saltar estos directorios
			if( ( !strcmp( ent->d_name, "." ) && !commands[ 7 ] ) || ( !strcmp( ent->d_name, ".." ) && !commands[ 7 ] ) ) continue; //Si

			//Si no esta definido el comando -A, saltar los elementos ocultos
			if( ent->d_name[ 0 ] == '.' && !commands[ 0 ] ) continue;

			//Esta funcion es la cual alimente struct stat st, en base al archivo que quiero saber.
			stat( ent->d_name, &st );

			//Si se quiere mostrar la informacion del nodo i
			if( commands[ 2 ] ){
				printf("%8d ", st.st_ino );
			}

			//Mostrar comando l
			if( commands[ 3 ] ){
				printf( "%10s %2d ", mode_human( st.st_mode ), st.st_nlink );

				//Si este comando esta activo, solo se muestra el user id
				if( commands[ 5 ] ){
					printf( "%4d ", st.st_uid );
				}
				else {
					//Mostrar los ID's en vez de los nombres de los owners
					if( commands[ 4 ] ){
						printf( "%4d %4d ", st.st_uid, st.st_gid );
					}
					else {
						struct passwd *pw = getpwuid( st.st_uid ); //Obtengo el nombre del owner
						struct group  *gr = getgrgid( st.st_gid ); //Obtengo el nombre del grupo

						printf( "%s %s ", pw->pw_name, gr->gr_name );
					}
				}

				//Si se quiere en formato entendible
				if( commands[ 1 ] ){
					printf( "%s ", file_size_human( st.st_size ) );
				}
				else {
					printf("%6d ", st.st_size );
				}

				printf( "%11s ", date_human( st.st_mtime ) ); //Imprimo la fecha
			}

			//En caso de que se quiera el size, y no se uso -l
			if( commands[ 1 ] && !commands[ 3 ] ){
				printf( " %s ", file_size_human( st.st_size ) );
			}

			//Por ultimo imprimo el nombre
			printf( "%s ", ent->d_name );

			//Si es un directorio y esta activo -p, poner / luego del nombre
			if( commands[ 6 ] ){
				if( st.st_mode & S_IFDIR ) printf( "/" );
			}

			if( !commands[ 9 ] ){
				printf("\n");
			}
		}
		closedir ( dir );
	}
	else {
		printf( "No se pudo abrir el directorio!\n" );
	}

	return 0;
}

//MOstrar el tamano del archivo en formato humano
char *file_size_human( int n ){
	char *r;
	r = (char *) malloc( 6 ); //Le asigno memoria para poder retornarlo y que no desaparezca
	
	int unit = 0; //Unit es el indice de la unidad que se va a representar( B, K, M, ... )
	
	double nd = n; //Truco para realizar las operaciones en flotante y dar un tamanio mas preciso.

	while( nd >= 1024 ){ //Mientras se pueda reducir la unidad
		unit++; //Cambio de unidad

		nd /= 1024; //Redusco el tamano de la unidad

		if( unit == 6 ){
			break; //Solo se soportan 7 unidades ( y mucho es ).
		}
	}
	
	//Uso sprintf para agregar un string de forma facil con el formato que quiero
	sprintf( r, "%.2f%c", nd, units[ unit ] );

	return r;
}

char *mode_human( int mask ){
	char *r;
	//En esta funcion simplemente se devuelve un string con el [tipo de archivo][y los permisos de: owner, grupo, otros ]

	/*
		Simplemente se realiza un & logico con la mascara especifica
	 	para ver si dicho usuario dispone de tal permiso
	 	Si no se tiene el permiso se muestra -
	 	EL primer caracter muestra el tipo del archivo '-' indica que es un fichero normal
	*/

	 r = (char *) malloc( 10 );
	strcpy( r, "----------" );

	if( S_IFDIR & mask ){ //Si es un directorio el primer caracter se muestra con d
		r[ 0 ] = 'd';
	}

	else if( S_IFLNK & mask ){ //Si es un syslink se muestra una l
		r[ 0 ] = 'l';
	}
	else {
		//Just in case
	}

	//PERMISOS OWNER
	if( S_IRUSR & mask ){
		r[ 1 ] = 'r';
	}

	if( S_IWUSR & mask ){
		r[ 2 ] = 'w';
	}

	if( S_IXUSR & mask ){
		r[ 3 ] = 'x';
	}

	//PERMISOS GROUP

	if( S_IRGRP & mask ){
		r[ 4 ] = 'r';
	}

	if( S_IWGRP & mask ){
		r[ 5 ] = 'w';
	}

	if( S_IXGRP & mask ){
		r[ 6 ] = 'x';
	}

	//PERMISOS OTROS
	if( S_IROTH & mask ){
		r[ 7 ] = 'r';
	}

	if( S_IWOTH & mask ){
		r[ 8 ] = 'w';
	}

	if( S_IXOTH & mask ){
		r[ 9 ] = 'x';
	}
	return r;
}


char *date_human( time_t fecha ){
	char *r;

	r = ( char * )malloc ( 12 );

	//strftime copia en un texto con el formato indicado de fecha
	//localtime devuelve una estructura con la informacion de fecha a la configuracion horaria actual del sistema
	strftime( r, 11, "%b %d %Y", localtime( &fecha ) );

	return r;
}
