#include <mysql.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <pthread.h>


typedef struct {
	char nombre[20];
	int socket;
}Tusuario;

typedef struct {
	Tusuario usuarios[100];
	int num; //numero de conectados
}TusuariosLista;

TusuariosLista usuarioLista; 
MYSQL *mysql_conn;    
pthread_mutex_t mutex;
int sockets[10];
int i;  

//Funcion para anadir ususarios a la lista sabiendo su nombre y su usuario
//Si no se puede anadir porque ya hay mas de 100 retorna -1
//Si se puede anadir, lo hace y retona 0
int anadirUsusario(char nombre[20], int socket, TusuariosLista *lista)
{ 
	if(lista->num == 100)
		return -1;
	else
	{
		strcpy(lista->usuarios[lista->num].nombre,nombre);
		lista->usuarios[lista->num].socket = socket;
		lista->num = lista->num + 1;
		return 0;
	}
}

//Procedimiento para saber los usuarios de la lista
//
void DameUsuarios(TusuariosLista *lista, char output[100])
{
	//string amb num,noms persones
	int i;
	sprintf(output,"6/%d,",lista->num);
	for (i=0; i < lista->num; i++)
		sprintf(output,"%s%s,",output,lista->usuarios[i].nombre);
}

//Funcion para saber la posicion de un usuario en  la lista sabiendo su nombre 
//Retorna la posicion del usuario en la lista si existe
//Si no existe retorna un -1
int DamePosicion(TusuariosLista *lista, char nombre[20])
{
	int found = 0;
	int i = 0;
	while(i < lista->num && !found)
	{
		if(strcmp(nombre,lista->usuarios[i].nombre)==0)
		{
			found = 1;
			return i;
		}
		i=i+1;
	}
	return -1;
}

//Funcion que nos indica el socket de un usuario sabiendo su nombre
//Si encuntra el usuario en la lista nos devuelve su socket
//Si no puede hacerlo nos devuelve un -1
int DameSocket(TusuariosLista *lista, char nombre[20])
{
	int found = 0;
	int i = 0;
	while(i < lista->num && !found)
	{
		if(strcmp(nombre,lista->usuarios[i].nombre)==0)
		{
			found = 1;
			return lista->usuarios[i].socket;
		}
		i=i+1;
	}
	return -1;
}
		
//Funcion que nos elimina a un jugador de la lista sabiendo su nombre
//Si el usuario existe lo elimina y devuelve un 0
//Si no encuentra ususario y no lo puede eliminar devuelve un -1 
int EliminaConectados(TusuariosLista *lista, char nombre[20])
{
	//borrem de la llista quan es desconecta
	int p = DamePosicion(lista,nombre);
	if (p==-1)
	return -1;
	else 
	{
		int i;
		for(i = p; i<lista->num-1; i++)
		{
			lista->usuarios[i]=lista->usuarios[i+1];
		}
		lista->num = lista->num - 1;
		return 0; 
	}	
}


//Funcion de login
//Consulta que el usuario exista en la base de datos 
//Si el usuario existe devuelve un 2
//Si el usuario no existe devuelve un 1
int login(char usuario[20],char password[20],MYSQL *conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	char consulta[512];
	//Consultar los datos son correctos
	strcpy(consulta,"SELECT jugador.usuario, jugador.pasword FROM jugador WHERE jugador.usuario='");
	strcat(consulta, usuario);
	strcat(consulta, "' AND jugador.pasword = '");
	strcat(consulta, password);
	strcat(consulta, "'");
	//Detectar errores en la consulta 
	err=mysql_query(conn,consulta);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}


//Funcion de SignUp
//Si puede anadir al jugador en la base de datos nos devuelve un 2
//Si no puede anadirlo devuelve un 1
int signup(char usuario[20],char password[20],MYSQL *conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	char consulta[512];
	//Consultar los datos son correctos
	strcpy(consulta,"INSERT INTO jugador(id,usuario,pasword) VALUES('");
	strcat(consulta, usuario);
	strcat(consulta, "','");
	strcat(consulta, password);
	strcat(consulta, "');");
	//Detectar errores en la consulta 
	err=mysql_query(conn,consulta);
	if(err!=0)
	{
		printf("Error al consultar la base de datos %u %s\n",mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}


//Funcion que nos indica los puntos totales de un jugador sabiendo su nombre
//Si encuentra el jugador devuelve su puntuacion
//Si no puede hacerlo devuelve un -1
int PuntosTotales(char jugador[20],MYSQL *conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hcaer la consulta
	char  consulta[512];
	sprintf(consulta,"SELECT SUM(relacion.puntuacion) FROM jugador, partida, relacion WHERE jugador.usuario ='%s' AND jugador.jugadorID = relacion.jugadorID AND relacion.partidaID = partida.partidaID",jugador);
	
	//Detectar errores en la consulta 
	err=mysql_query(conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}


//Funcion que nos indica le tiempo total de juego de un jugador sabiendo su nombre
//Si encuentra el jugador devuelve su tiempo total
//Si no puede hacerlo devuelve un -1
int TiempoTotal(char jugador[20],MYSQL *conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hacer la consulta
	char consulta [512];
	sprintf(consulta,"SELECT SUM(partida.duracion) FROM jugador, partida, relacion WHERE jugador.usuario = '%s' AND partida.partidaID = relacion.partidaID AND relacion.jugadorID = jugador.jugadorID", jugador);
	
	//Detectar errores en la consulta 
	err=mysql_query(conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}

//Funcion que nos indica cuantas veces un jugador a ganado a otro
//Si encuentra las partidas entre ambos devuelve el numero de vecesa ganadas
//Si no puede hacerlo devuelve un -1
int VecesGanadas(char jugador1[20],char jugador2[20],MYSQL *conn)
{
	int err;
	MYSQL_RES *result;
	MYSQL_ROW row;
	
	//Hcaer la consulta
	char consulta [512];
	sprintf(consulta, "SELECT COUNT(partida.ganador) from partida WHERE partida.ganador='%s' AND partida.jugador2='%s'",jugador1,jugador2);
	
	//Detectar errores en la consulta 
	err=mysql_query(conn,consulta);
	if(err!=0)
	{
		printf(consulta,"Error al consultar la base de datos %u %s\n",mysql_errno(conn),mysql_error(conn));
		exit(1);
	}
	//Conseguir los resultados
	result=mysql_store_result(conn);
	row=mysql_fetch_row(result);
	if(row==NULL)
	{
		return -1;
	}
	else
	{
		int result=atoi(row[0]);
		return result;
	}
}

void *AtenderCliente(void *socket)
{	
	int sock_conn;
	sock_conn = *(int *) socket;
	
	MYSQL_ROW row;
	char buff[512];
	char respuesta[512];
	char buff3[512];
	char notification[512];
	char user[20];
	char userme[20];
	char pass[20];
	char jugador1[20];
	char jugador2[20];
	int  ret;
	char invitado[20];
	char chatmensaje[80];
	
	int fin=0;
	while(fin==0) 
	{
		//Recibimos el mensaje de cliente
		ret=read(sock_conn,buff, sizeof(buff)); 
		printf ("Recibido\n");
		
		//Anadimos la marca final de string para no escribir despues del buffer
		buff[ret]='\0'; 
		printf ("Se ha conectado: %s\n",buff);
		
		//Arrancamos el codigo del mensaje
		char *p = strtok( buff, "/");
		int codigo =  atoi (p);
		
		//Analizamos para cada codigo
		//Desconectar
		if(codigo == 0) 
		{
			fin=1;
			//Eliminamos el jugador de la lista
			EliminaConectados(&usuarioLista,user);
			//Actualizamos la lista
			DameUsuarios(&usuarioLista,notification); 
			int j;
			//Notificamos la nueva lista a todos los demas
			for (j=0;j<usuarioLista.num;j++) 
				write(usuarioLista.usuarios[j].socket,notification,strlen(notification)); 
		}
		//Log In
		if(codigo == 1)
		{
			//Arrancamos el usuario del mensaje
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Arrancamos la contrasena del mensaje
			p = strtok( NULL, "/"); 
			strcpy (pass,p);
			//Comprobamos que existe el usuario en la base de datos
			int res=login(user,pass,mysql_conn);
			//Si el usuario no existe
			if(res==1)
			{
				//Informamos que no ha sido posible el log in
				sprintf(respuesta,"1/2");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			//Si el usuasrio si existe
			else
			{
				//Informamos que ha sido posible hacer el log in
				sprintf(respuesta,"1/1");
				write (sock_conn,respuesta, strlen(respuesta));
				pthread_mutex_lock (&mutex);
				strcpy(userme,user);
				//Anadimos el usuario en la lista de conectados
				anadirUsusario(userme, sock_conn, &usuarioLista); 
				pthread_mutex_unlock (&mutex);
				//Actualozamos la lista de conectados
				DameUsuarios(&usuarioLista,buff3); 
				int j;
				//Notificamos a todos los usuarios conectados con la nueva lista
				for (j=0;j<usuarioLista.num;j++)  
					write(usuarioLista.usuarios[j].socket,buff3,strlen(buff3));
			}
		}
		//SignUp
		if(codigo == 2)
		{
			//Arrancamos el usuario del mensaje
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Arrancamos la contrasena del mensaje
			p = strtok( NULL, "/"); 
			strcpy (pass,p);
			//Comprobar si el usuario ya existe
			int res=login(user,pass,mysql_conn);
			//Si el usuario no existe
			if(res==1)
			{
				//Registramos el ususario
				pthread_mutex_lock (&mutex);
				int sign = signup(user,pass,mysql_conn);
				//Si no se ha podido registrar 
				if(sign==1)
				{
					//Informamos que no ha sido registrado
					sprintf(respuesta,"2/");
					write (sock_conn,respuesta, strlen(respuesta));
				}
				//Si se ha podido registrar
				else
				{
					//Informamos que ha podido ser registrado
					sprintf(respuesta,"2/1");
					write (sock_conn,respuesta, strlen(respuesta));
					pthread_mutex_unlock (&mutex);
				}
			}
			//Si el usuario ya existe
			else 
			{
				//Informamos que no ha sido posible registarse
				sprintf(respuesta,"2/");
			}
		}
		//PuntosTotales
		if(codigo == 3)
		{
			//Arrancamos el ususario
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Obtenemos los puntos totales
			int res=PuntosTotales(user,mysql_conn);
			//Ha habido algun error
			if(res==-1)
			{
				//Informamos que la consulta ha sido fallida
				sprintf(respuesta,"3/");
				write (sock_conn,respuesta, strlen(respuesta));	
			}
			//Se han obtenido los puntos totales
			else
			{
				//Informamos de los puntos totales
				sprintf(respuesta,"3/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		//Tiempo Total
		if(codigo == 4)/
		{
			//Arrancamos el usuario
			p = strtok( NULL, "/"); 
			strcpy (user, p);
			//Obtenemos el tiempo total del jugador
			int res=PuntosTotales(user,mysql_conn);
			//Si no se ha obtenido
			if(res==-1)
			{
				//Informamos que la consulta no ha tenido exito
				sprintf(respuesta,"4/");
				write (sock_conn,respuesta, strlen(respuesta));	
			}
			//Si se obtiene el tiempo total
			else
			{
				//Informamos del tiempo total del jugador
				sprintf(respuesta,"4/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));
			}
		}
		//Cuantas veces uno a ganado a otro
		if(codigo == 5)
		{
			//Arrancamos el nombre del jugador ganador
			p = strtok( NULL, "/"); 
			strcpy (jugador1, p);
			//Arrancamos el nombre del jugador perdedor
			p = strtok( NULL, "/"); 
			strcpy (jugador2,p);
			//Obtenemos las veces que el jugador ha ganado al otro
			int res=VecesGanadas(jugador1,jugador2,mysql_conn);
			//Si la consulta no ha sido posible
			if(res==-1)
			{
				//Informamos que ha habido un error
				sprintf(respuesta,"5/");
				write (sock_conn,respuesta, strlen(respuesta));
			}
			//Si la consulta ha sido posible
			else
			{
				//Informamos de las vecesa ganadas
				sprintf(respuesta,"5/%d",res);
				write (sock_conn,respuesta, strlen(respuesta));	
			}
		}
		//Enviar invitacion para jugar a un jugador
		if (codigo==7) 
		{
			//Arrancamos el nombre del jugador que quieren invitar
			p = strtok(NULL,"/");
			strcpy(invitado,p);
			//Comprobamos que el jugador existe
			int socketInvitado = DameSocket(&usuarioLista,invitado);
			//Si el jugador existe
			if(socketInvitado>0)
			{
				//Enviamos al invitado la invitacion
				sprintf(respuesta,"8/%s",userme); 
				printf("La respuesta que se envia es: %s\n",respuesta);
				write(socketInvitado,respuesta,strlen(respuesta));
			}
		}
		//El jugador invitado envia la respuesta al invitador
		if(codigo==9)
		{
			char invitacionAnswer[20];
			//Arrrancamos la respuesta del invitado
			p = strtok(NULL,"/");
			strcpy(invitacionAnswer,p); 
			//Si la respuesta es un SI
			if (strcmp(invitacionAnswer,"SI")==0)
			{
				//Informamos al invitador que se ha acceptado su invitacion
				printf("He entrado en SI");
				sprintf(respuesta,"10/%s/",invitacionAnswer); 
				write(sock_conn,respuesta,strlen(respuesta));
			}
			//Si la respuesta es un NO
			else
			{
				//Informamos al invitador que se ha rechazado la invitacion
				printf("He entrado en NO");
				sprintf(respuesta,"10/%s/",invitacionAnswer); 
				write(sock_conn,respuesta,strlen(respuesta));
			}
		}
		//Enviamos un mensaje al oponente
		if(codigo==11)
		{
			//Arrancamos el nombre del oponente
			p = strtok(NULL,"/");
			//Arrancamos el mensaje que queremos enviar
			strcpy(invitado,p);
			p = strtok(NULL, "/");
			strcpy(chatmensaje,p);
			//Comprobamos que el oponente exista y obtenemos sus socket
			int socketInvitado = DameSocket(&usuarioLista,invitado);
			//Si el oponente existe 
			if(socketInvitado>0)
			{
				//Le enviamos el mensaje para el
				sprintf(respuesta,"11/%s/%s/",userme,chatmensaje); //  17/invitador,
				printf("La respuesta que se envia es: %s\n",respuesta);
				write(socketInvitado,respuesta,strlen(respuesta));
			}
		}
	}
}
	
int main(int argc, char *argv[])
{
	MYSQL_ROW row;
	int sock_conn, sock_listen, ret;
	struct sockaddr_in serv_adr;
			
	// INICIALITZACIONS:
	//Obrim i inicialitzem connexi? amb mysql
	mysql_conn = mysql_init(NULL);
	if (mysql_conn==NULL)
	{
		printf("Error al crear la conexion: %u %s\n", mysql_errno(mysql_conn), mysql_error(mysql_conn));
		exit(1);
	}
	mysql_conn = mysql_real_connect(mysql_conn,"shiva2.upc.es","root","mysql","isma13",0,NULL,0);
	if (mysql_conn==NULL)
	{
		printf ("Error al inicializar la conexion: %u %s\n",mysql_errno(mysql_conn),mysql_error(mysql_conn));
		exit(1);
	}
			
	// Obrim el socket
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
		printf("Error creant socket");
	// Fem el bind al port
	memset(&serv_adr, 0, sizeof(serv_adr));// inicialitza a zero serv_addr
	serv_adr.sin_family = AF_INET;
			
	// asocia el socket a cualquiera de las IP de la m?quina. 
	//htonl formatea el numero que recibe al formato necesario
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	// escucharemos en el port 50001
	serv_adr.sin_port = htons(50001);
	if (bind(sock_listen, (struct sockaddr *) &serv_adr, sizeof(serv_adr)) < 0)
		printf ("Error al bind");
	//La cola de peticiones pendientes no podr? ser superior a 4
	if (listen(sock_listen, 2) < 0)
		printf("Error en el Listen");
	
	pthread_t thread[100]; //estructura especial definida a la llibreria 
	//nomes puc crear 100 thread
	
	//***Observar si esto es lo que falla***
	int sockets[10];
	i = 0;
	for(;;)
	{
		printf ("Escuchando\n");
		sock_conn = accept(sock_listen, NULL, NULL);
		sockets[i] = sock_conn;
				
		printf ("He recibido conexi?n\n");
		pthread_create( &thread[i], 
		NULL, 
		AtenderCliente, //nom de la funcio a ajecutar
		&sockets[i]); //socket per entregar al threat
		i=i+1;
	}
	mysql_close (mysql_conn);
}
