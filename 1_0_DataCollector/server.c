#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <mysql.h>
#include <my_global.h>
#include <time.h>

#define BUF_SIZE 100
int sig_sock;

char UserID[16];
char UserPW[16];
char Database[32];

void finish_with_error(MYSQL *con);
void Set_DB_User(char *id,char* pw);
void Set_Database(char *db_name);
void SendQuery(char* a_query);
void finish_with_error(MYSQL *con);


void my_sig_handler(int sig)
{
	printf("\nstop service! \n");

	close(sig_sock);
	exit(1);
}
void my_error_handling(char * message)
{
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}
int GetFrameData(int a_fd_socket, unsigned char *ap_message_id, char **ap_body_data, unsigned char *ap_body_size)
{
	int temp_size, state = -1;
	char key;

	temp_size = read(a_fd_socket, &key, 1);
	if(temp_size < 0)
	{
		printf("클라이언트 연결 끊어짐\n");
	}
	else if(key == 27)
	{
		temp_size = read(a_fd_socket, ap_message_id, 1);
		if(temp_size < 0)
		{
			printf("클라이언트 연결 끊어짐\n");
		}
		else if(temp_size ==0 )
		{
			printf("%d 클라이언트가 종료되었습니다. \n",a_fd_socket);
			state =-2;
		}
		else
		{
			temp_size = read(a_fd_socket, ap_body_size, 1);
			if(temp_size < 0)
			{
				printf("클라이언트 연결 끊어짐\n");
			}
			else
			{

				if(*ap_body_size > 0)
				{
					*ap_body_data = (char *)malloc(*ap_body_size);
					temp_size = read(a_fd_socket, *ap_body_data, *ap_body_size);
					if(temp_size < 0)
					{
						printf("클라이언트 연결 끊어짐\n");
					}
					else
					{
						return 1;
					}
				}
				else
				{
					*ap_body_data = NULL;
					return 1;
				}
			}
		}
	}
	else
	{
		printf("유효하지 않은 클라이언트\n");
		state = -2;
	}
	return state;
}
int main()
{
	int serv_sock, clnt_sock;
	int str_len, status,fd_num,i,fd_max,get_msg;

	struct sockaddr_in serv_adr,clnt_adr;
	struct timeval timeout;
	struct sigaction act;
	socklen_t adr_sz;

	fd_set reads, cpy_reads;
	char buf[BUF_SIZE];
	unsigned char message_id,body_size;
	char *p_body_data;

	int tnum =0;
	double value;
	char* trash;

	//sigaction 함수 부분.
	act.sa_handler = my_sig_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags=0;
	status = sigaction(SIGINT,&act,NULL);

	//소켓 생성
	serv_sock = socket(PF_INET,SOCK_STREAM,0);
	if(serv_sock < 0 )
		printf("Error in socket function!! \n");
	sig_sock = serv_sock;
	//소켓 등록
	memset(&serv_adr,0,sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serv_adr.sin_port=htons(25002);

	//소켓에 주소 등록.
	if(bind(serv_sock,(struct sockaddr*)&serv_adr,sizeof(serv_adr))==-1)
		my_error_handling("bind function error!");
	//리슨 상태
	if(listen(serv_sock,5)==-1)
		my_error_handling("listen function error!");

	FD_ZERO(&reads);
	FD_SET(serv_sock,&reads);
	fd_max = serv_sock;

	char strQuery[128];
	time_t t;
	struct tm* pt;
	//구현
	while(1)
	{
		printf("Waiting for new Client...\n");
		cpy_reads = reads;
		timeout.tv_sec = 5;
		timeout.tv_usec = 5000;

		if((fd_num = select(fd_max +1,&cpy_reads,0,0,&timeout))==-1)
			break;
		if(fd_num ==0)
			continue;

		for(i=0;i<fd_max+1;i++)
		{
			if(FD_ISSET(i,&cpy_reads))
			{
				if(i == serv_sock) //connection request!!
				{
					adr_sz = sizeof(clnt_adr);
					clnt_sock = accept(serv_sock,(struct sockaddr*)&clnt_adr,&adr_sz);
					FD_SET(clnt_sock,&reads);
					if(clnt_sock > fd_max)
						fd_max = clnt_sock;
					printf("Connected client : %d \n",clnt_sock);
				}
				else // read message!!
				{

					get_msg= GetFrameData(clnt_sock, &message_id, &p_body_data, &body_size);
					value = strtod(p_body_data,&trash);
					if(get_msg==1)
					{
						if(p_body_data != NULL)
						{
							memset(strQuery,0,sizeof(strQuery));
							Set_DB_User("root","asdf12");
							Set_Database("testdb");
							time(&t);
							pt = localtime(&t);

							if(message_id ==2)//온도 부분.
							{
								sprintf(strQuery,"INSERT INTO Temperature VALUES(%d,'%d-%d-%d %d:%d:%d',%.2f)",clnt_sock,pt->tm_year+1900,pt->tm_mon+1,pt->tm_mday,pt->tm_hour,pt->tm_min,pt->tm_sec,value);
								printf("Query : %s \n",strQuery);
								SendQuery(strQuery);

							}
							else if(message_id == 4) // 습도 부분.
							{
							       sprintf(strQuery,"INSERT INTO Humidity VALUES(%d,'%d-%d-%d %d:%d:%d,%.2f')",clnt_sock,pt->tm_year+1900,pt->tm_mon+1,pt->tm_mday,pt->tm_hour,pt->tm_min,pt->tm_sec,value);

								printf("Query : %s \n",strQuery);
								SendQuery(strQuery);
							}
							printf("client : %s \n",p_body_data);
							free(p_body_data);

						}
					}
					else
					{
						if(get_msg ==-2)
						{
							FD_CLR(i,&reads);
							close(i);
							printf("close client: %d \n",i);
							break;
						}
					}//else
				}//read message else
			}
		}
	}
	return 0;
}

void Set_DB_User(char* id, char* pw)
{
	memset(UserID, 0, sizeof(UserID));
	memset(UserPW, 0, sizeof(UserPW));
	if(id != NULL)  memcpy(UserID, id, strlen(id));
	if(pw != NULL)  memcpy(UserPW, pw, strlen(pw));
}

void Set_Database(char* db_name)
{
	memset(Database, 0, sizeof(Database));
	if(db_name != NULL)     memcpy(Database, db_name, strlen(db_name));
}

void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);
	exit(1);
}

void SendQuery(char* a_query)
{
	if(Database == NULL)            printf("(!) Set 'Database' first\n");
	if(UserID == NULL)                      printf("(!) Set 'User ID of DB' first\n");
	if(UserPW == NULL)                      printf("(!) Set 'User PW of DB' first\n");

	MYSQL *con = mysql_init(NULL);
	if (con == NULL)
	{
		fprintf(stderr, "%s \n", mysql_error(con));
		exit(1);
	}

	if (mysql_real_connect(con, "localhost", UserID, UserPW, Database, 0, NULL, 0) == NULL)
		finish_with_error(con);
	if (mysql_query(con, a_query))
		finish_with_error(con);

	mysql_close(con);
}
