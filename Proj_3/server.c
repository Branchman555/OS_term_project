#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <sys/types.h>  
#include <sys/stat.h>  
#include <sys/socket.h>  
#include <signal.h>  
#include <unistd.h>  
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <sys/ipc.h>
#include <sys/mman.h>


#define SERV_IP		"220.149.128.100"
#define SERV_PORT	4051	
#define BACKLOG		10
#define MAXLINE 1024 

#define USER1	"ZEP"
#define USER2	"LEO"
#define USER3	"CLAUS"
#define USER4	"CHAIN"
#define USER5	"STEVEN"

#define PWD1	"BB1"
#define PWD2	"BB2"
#define PWD3	"BB3"
#define PWD4	"BB4"
#define PWD5	"BB5"

//클라이언트로 보낼 허용, 거부 메시지
#define WELCOM_MSG	"1===== Welcome To OpenChatting Room!! ====="
#define DENIED_MSG	"0===== You're Not Allowed User ====="
int FindUser(char* target_name, char index[5][10]);

//유저의 아이디와 비밀번호를 찾기위한 함수
//타켓이 목록에 있다면 목록 번호를 출력한다.
int FindUser(char* target_name, char index[5][10]) {
	int user_num = 0;
	for (user_num = 0; user_num < 5; user_num++) {
		if (strstr(target_name, index[user_num]) != NULL) {
			return user_num;
		}
	}
	return -1;
}

int main()
{
	int rcv_byte;
	int sockfd, new_fd;
	int client_count = 0;
	int state, client_len;
	pid_t pid;
	unsigned int sin_size;
	int val = 1;
	int client_num = 0;
	unsigned int msg_flag = 0;

	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;

	char UserID[5][10] =
	{
		USER1,
		USER2,
		USER3,
		USER4,
		USER5
	};

	char UserPwd[5][10] =
	{
		PWD1,
		PWD2,
		PWD3,
		PWD4,
		PWD5
	};

	char wel_msg[50] = WELCOM_MSG;
	char dny_msg[50] = DENIED_MSG;
	char ID_buf[10];
	char PW_buf[10];
	char buf[MAXLINE];
	char sharedBuf[MAXLINE];
	
	//유저 접속정보를 공유하기 위한 메모리
	int* UserIn = mmap(NULL, sizeof(int) * 5, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//프로세스 종료를 위한 공유메모리를 사용한 플래그
	char* shmaddr = mmap(NULL, sizeof(char) * 100, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//각 유저의 IP를 저장하는 공유메모리 배열
	char** ipAddress = mmap(NULL, sizeof(char*) * 10, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	for (int i = 0; i < 10; i++) {
		ipAddress[i] = mmap(NULL, sizeof(char) * 16, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Sercer-socket() error lol!");
		exit(1);
	}
	else printf("Server-socket() sockfd is OK...\n");
	my_addr.sin_family = AF_INET;

	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("setsockopt");
		close(sockfd);
		return -1;
	}

	if (bind(sockfd, (struct sockaddr*) & my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Server=bind() error lol!");
		exit(1);
	}
	else printf("Server-bind() is OK...\n");

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen() error lol!");
		exit(1);
	}
	else printf("listen() is OK...\n\n");

	sin_size = sizeof(struct sockaddr_in);

	signal(SIGCHLD, SIG_IGN);
	shmaddr[0] = '1';
	sharedBuf[0] = '1';
	shmaddr[1] = '\0';
	sharedBuf[1] = '\0';

	while (1)
	{
		new_fd = accept(sockfd, (struct sockaddr*) & their_addr, &sin_size);
		if (new_fd == -1)
		{
			perror("Accept error : ");
			exit(0);
		}
		printf("Accept Success!!\n");

		//클라인트가 접속되는 순간 Fork한다.
		//자식서버 시작
		pid = fork();
		if (pid == 0)
		{
			printf("Child_process:%d\n", client_num);
			printf(">>");
			memset(buf, 0x00, MAXLINE);

			//클라이언트로 부터 ID수신
			rcv_byte = recv(new_fd, ID_buf, sizeof(ID_buf), 0);
			if (rcv_byte <= 0)	exit(0);
			else	printf("recv:%s\n", ID_buf);

			//클라이언트로 부터 PW수신
			rcv_byte = recv(new_fd, PW_buf, sizeof(PW_buf), 0);
			if (rcv_byte <= 0)	exit(0);
			else	printf("recv:%s\n", PW_buf);

			//ID PW를 비교하여 클라이언트 접속을 허용할 것인지 판별
			if (FindUser(ID_buf, UserID) == -1) {
				send(new_fd, dny_msg, strlen(dny_msg) + 1, 0);
				exit(0);
			}
			else {
				if (FindUser(ID_buf, UserID) == FindUser(PW_buf, UserPwd)) {
					;
					client_num = FindUser(ID_buf, UserID);
					if (UserIn[client_num] == 1) {
						send(new_fd, dny_msg, strlen(dny_msg) + 1, 0);
						exit(0);
					}
					else {
						send(new_fd, wel_msg, strlen(wel_msg) + 1, 0);
						UserIn[client_num] = 1;
					}
				}
				else {
					send(new_fd, dny_msg, strlen(dny_msg) + 1, 0);
					exit(0);
				}
			}

			printf("%d\n", client_num);
			//접속한 클라이언트의 번호를 찾아서 해당 번호에 접속 IP를 기록한다
			strcpy(ipAddress[client_num], inet_ntoa(their_addr.sin_addr));
			printf("Client IP: %s, %d\n", ipAddress[client_num], (int)strlen(ipAddress[client_num]));

			strcpy(sharedBuf, shmaddr);

			//클라이언트와 통신할 자식 서버의 새로운 분기점
			pid = fork();
			if (pid > 0) {
				//메시지를 echo하는 부분
				//공유메모리를 상시 모니터링하여 변화가 감지되면 메시지를 클라이언트로 전송
				while (1) {
					while (!strcmp(shmaddr, sharedBuf)) {
						usleep(100);
					}
					strcpy(sharedBuf, shmaddr);

					//메시지를 받은 클라이언트를 제외하고 메시지를 ECHO
					if (shmaddr[1] != (client_num + 48)) {
						printf("%s\n", shmaddr);
						//파일 전송 요청이 왔는지 체크
						if (strstr(shmaddr, "FILE") != NULL) {
							//만일 파일 전송 요청이 왔다면 요청할 클라이언트에 한해서 파일 전송
							if (FindUser(strstr(shmaddr, "FILE"), UserID) == client_num) {
								printf("%s\n", strstr(shmaddr, "FILE"));
								send(new_fd, shmaddr + 2, strlen(shmaddr) - 1, 0);
							}
						}
						else {
							send(new_fd, shmaddr + 2, strlen(shmaddr) - 1, 0);
						}
					}
				}
			}
			else if (pid == 0) {
				//메시지를 받는부분
				while (1) {
					rcv_byte = recv(new_fd, buf, sizeof(buf), 0);
					if (rcv_byte <= 0)	continue;
					else	printf("%d=recv:%s\n", client_num, buf);

					//파일 전송 요청이 왔다면
					if (strstr(buf, "FILE") != NULL) {
						//요청할 유저의 이름을 검색
						int temp = FindUser(strstr(buf, "FILE"), UserID);
						//유저가 목록에 없다면 NOUSER메시지 전송
						if (temp == -1) {
							send(new_fd, "NOUSER\0", 7, 0);
							continue;
						}
						//유저가 목록에 있으나 접속하지 않았을경우 NOUSER메시지 전송
						else if (UserIn[temp] == 0) {
							send(new_fd, "NOUSER\0", 7, 0);
							continue;
						}
						//파일 전송 요청할 유저 IP를 전송
						else {
							send(new_fd, ipAddress[temp], strlen(ipAddress[temp]), 0);
						}
					}
					//공유메모리에 메시지 전송
					sprintf(shmaddr, "%d%d[%s]: %s", (msg_flag++ % 10), client_num, UserID[client_num], buf);
					if (!strcmp(buf, "exit\0")) {
						printf("exited\n");
						UserIn[client_num] = 0;
						send(new_fd, "Bye\0", 4, 0);
						close(new_fd);
						exit(0);
						break;
					}
				}
			}
			if (pid == -1)
			{
				printf("error\n");
				perror("fork error : ");
				return 1;
			}
			if (!strcmp(buf, "exit")) {
				close(new_fd);
				break;
			}
		}
	}
	close(new_fd);
	return 0;
}
