#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <mysql.h>
#include <my_global.h>

char UserID[16];
char UserPW[16];
char Database[32];

char DataRow[10][100];

#define HOST_PORT 25602
#define BROADCAST_LIST  5
char IP[BROADCAST_LIST][] = {
    "192.168.0.2",
    "192.168.0.3",
    "192.168.0.4",
    "192.168.0.5",
    "192.168.0.6"
}

enum
{
    TEMPERATURE = 0,
    HUMIDITY,
    LIGHT,
}

bool g_ctrl_c_pressed = false;
void my_sig_handler(int a_signal)
{
    write(0, "\nCtrl^C pressed in sig handler\n", 32);
    g_ctrl_c_pressed = true;
}

void    Set_DB_User(char* id, char* pw);
void    Set_Database(char* db_name);
int     IsUpdated(int* last_id_saved);
int     SendFrameData(int a_fd_socket, unsigned char a_message_id, const char *ap_body_data, unsigned char a_body_size);
int     GetFrameData(int a_fd_socket, unsigned char *ap_message_id, char **ap_body_data, unsigned char *ap_body_size);

int main(void)
{
    fd_set rfds;
    struct timeval tv;
    unsigned char message_id, body_size;
    char *p_body_data;
    int retval;

    struct sockaddr_in serv_addr;
    struct sigaction sig_struct;
    sig_struct.sa_handler = my_sig_handler;
    sig_struct.sa_flags = 0;
    sigemptyset(&sig_struct.sa_mask);

    if (sigaction(SIGINT, &sig_struct, NULL) == -1)
    {
        printf("Problem with sigaction\n");
        exit(1);
    }

    int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_socket < 0)
    {
        printf("Socket system error!!\n");
        return 0;
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(HOST_PORT);

    int     Select = 0;
    char    DB_Name[][] = {"Temperature", "Humidity", "Light"}
    int     DB_LastID[3] = {0, 0, 0};

    //TODO ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
    //데이터베이스 세팅
    Set_DB_User("root", "tips20!7");
    //(사전)온도테이블 컬럼 작성하기
    // CREATE TABLE Temperature (Id Int PRIMARY_KEY AUTO_INCREMENT, Name TEXT)
    //★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★

    //데이터베이스 주기적(1sec) 체크
    while(1)
    {
        Set_Database(Database_Name[Select]);

        //데이터베이스에 변동사항 있을시
        if(IsUpdated(DB_LastID[Select]) != 0)
        {
            //  1. 데이터 읽어와서
            sprintf(strQuery, "SELECT * FROM %s WHERE id IN (%d);", DB_Name[Select], DB_LastID[Select]);
            SendQuery(strQuery, true);

            //  2. IP리스트에 뿌려줌
            int server_idx = 0;
            while(server_idx < BROADCAST_LIST)
            {
                serv_addr.sin_addr.s_addr = inet_addr(IP[server_idx]);

                if (connect(fd_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
                {// 비정상 루틴 -> 재시도
                    printf("Connect error!!\n");
                    close(fd_socket);

                    fd_socket = socket(AF_INET, SOCK_STREAM, 0);
                    if (fd_socket < 0)
                    {
                        printf("Socket system error!!\n");
                        return 0;
                    }
                
                    bzero((char *)&serv_addr, sizeof(serv_addr));
                    
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = htons(HOST_PORT);          //NEED CHECK
                    continue;
                }
                else
                {// 정상 루틴
                    tv.tv_sec = 0;
                    tv.tv_usec = 50000;
                    FD_ZERO(&rfds);
                    FD_SET((long unsigned int)fd_socket, &rfds);
                    
                    //TODO ★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
                    //테이블마다 데이터필드 다를 경우, 데이터 저장된 위치도 다를 수 있음
                    //콘솔에 찍어보기
                    switch(Select)
                    {
                    case TEMPERATURE:
                        SendFrameData(fd_socket, 0x01, DataRow[2], sizeof(DataRow[2]));
                        break;
        
                    case HUMIDITY:
                        SendFrameData(fd_socket, 0x02, DataRow[2], sizeof(DataRow[2]));
                        break;
        
                    case LIGHT:
                        SendFrameData(fd_socket, 0x04, DataRow[2], sizeof(DataRow[2]));
                        break;
                    }
                    //★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★★
                    close(fd_socket);
                    server_idx++;
                }
            }
        }

        if (g_ctrl_c_pressed)
        {
            printf("Ctrl^C Pressed\n");
            printf("Exit Program\n");
            break;
        }
        
        Select++;
        Select%=3;
        sleep(1);
    }

    return 0;
}

void Set_DB_User(char* id, char* pw)
{
	memset(UserID, 0, sizeof(UserID));
	memset(UserPW, 0, sizeof(UserPW));
	if(id != NULL)	memcpy(UserID, id, strlen(id));
	if(pw != NULL)	memcpy(UserPW, pw, strlen(pw));
}

void Set_Database(char* db_name)
{
	memset(Database, 0, sizeof(Database));
	if(db_name != NULL)	memcpy(Database, db_name, strlen(db_name));
}

void finish_with_error(MYSQL *con)
{
	fprintf(stderr, "%s\n", mysql_error(con));
	mysql_close(con);
	exit(1);        
}

int IsUpdated(int* last_id_saved)
{
    int last_id = -1;

	if(Database == NULL)		printf("(!) Set 'Database' first\n");
	if(UserID == NULL)			printf("(!) Set 'User ID of DB' first\n");
    if(UserPW == NULL)			printf("(!) Set 'User PW of DB' first\n");
    if(Database == NULL || UserID == NULL || UserPW == NULL)    return -1;

	MYSQL *con = mysql_init(NULL);
	if (con == NULL) 
	{
		fprintf(stderr, "%s \n", mysql_error(con));
		exit(1);
	}

	if (mysql_real_connect(con, "localhost", UserID, UserPW, Database, 0, NULL, 0) == NULL)
        finish_with_error(con);
        
	last_id = mysql_insert_id(con)
    mysql_close(con);

    if(*last_id_saved != last_id)
    {
        *last_id_saved = last_id;
        return *last_id_saved;
    }
    else
        return 0;
}

void SendQuery(char* a_query, bool need_result)
{
    if(Database == NULL)		printf("(!) Set 'Database' first\n");
    if(UserID == NULL)			printf("(!) Set 'User ID of DB' first\n");
    if(UserPW == NULL)			printf("(!) Set 'User PW of DB' first\n");
    if(Database == NULL || UserID == NULL || UserPW == NULL)    return -1;

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

    if(need_result)
    {
        MYSQL_ROW row;
        MYSQL_RES* result = mysql_store_result(con);
        if(result == NULL)      finish_with_error(con);
        int num_fields = mysql_num_fields(result);
        
        memset(DataRow, 0x00, sizeof(DataRow));

        int i = 0;
        while(row = mysql_fetch_row(result))
        {
            for(int i=0; i< num_fields; i++)
            {
                sprintf(DataRow[i], "%s", row[i]? row[i] : "NULL");
            }
        }

        mysql_free_result(result);
        mysql_close(con);
    }

    mysql_close(con);
}

int SendFrameData(int a_fd_socket, unsigned char a_message_id, const char *ap_body_data, unsigned char a_body_size)
{
    char *p_send_data = (char *)malloc(3 + a_body_size);

    *p_send_data = 27;
    *(unsigned char *)(p_send_data + 1) = a_message_id;
    *(unsigned char *)(p_send_data + 2) = a_body_size;

    memcpy(p_send_data + 3, ap_body_data, a_body_size);

    int temp_size = write(a_fd_socket, p_send_data, 3 + a_body_size);
    if (temp_size < 0)
    {
        printf("Disconnected Client!!\n");
        close(a_fd_socket);
        return -1;
    }

    return 1;
}

int GetFrameData(int a_fd_socket, unsigned char *ap_message_id, char **ap_body_data, unsigned char *ap_body_size)
{
    int temp_size, state = -1;
    char key;
    temp_size = read(a_fd_socket, &key, 1);
    if (temp_size < 0)
    {
        printf("Disconnected Client!!\n");
    }
    else if (key == 27)
    {
        temp_size = read(a_fd_socket, ap_message_id, 1);
        if (temp_size < 0)
        {
            printf("Disconnected Client!!\n");
        }
        else
        {
            temp_size = read(a_fd_socket, ap_body_size, 1);
            if (temp_size < 0)
            {
                printf("Disconnected Client!!\n");
            }
            else
            {
                if (*ap_body_size > 0)
                {
                    *ap_body_data = (char *)malloc(*ap_body_size);
                    temp_size = read(a_fd_socket, *ap_body_data, *ap_body_size);
                    if (temp_size < 0)
                    {
                        printf("Disconnected Client!!\n");
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
        printf("Invalid Client!!\n");
        state = -2;
    }

    close(a_fd_socket);
    return state;
}