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

//Ŭ���̾�Ʈ�� ���� ���, �ź� �޽���
#define WELCOM_MSG	"1===== Welcome To OpenChatting Room!! ====="
#define DENIED_MSG	"0===== You're Not Allowed User ====="
int FindUser(char* target_name, char index[5][10]);

//������ ���̵�� ��й�ȣ�� ã������ �Լ�
//Ÿ���� ��Ͽ� �ִٸ� ��� ��ȣ�� ����Ѵ�.
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
	
	//���� ���������� �����ϱ� ���� �޸�
	int* UserIn = mmap(NULL, sizeof(int) * 5, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//���μ��� ���Ḧ ���� �����޸𸮸� ����� �÷���
	char* shmaddr = mmap(NULL, sizeof(char) * 100, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	//�� ������ IP�� �����ϴ� �����޸� �迭
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

		//Ŭ����Ʈ�� ���ӵǴ� ���� Fork�Ѵ�.
		//�ڽļ��� ����
		pid = fork();
		if (pid == 0)
		{
			printf("Child_process:%d\n", client_num);
			printf(">>");
			memset(buf, 0x00, MAXLINE);

			//Ŭ���̾�Ʈ�� ���� ID����
			rcv_byte = recv(new_fd, ID_buf, sizeof(ID_buf), 0);
			if (rcv_byte <= 0)	exit(0);
			else	printf("recv:%s\n", ID_buf);

			//Ŭ���̾�Ʈ�� ���� PW����
			rcv_byte = recv(new_fd, PW_buf, sizeof(PW_buf), 0);
			if (rcv_byte <= 0)	exit(0);
			else	printf("recv:%s\n", PW_buf);

			//ID PW�� ���Ͽ� Ŭ���̾�Ʈ ������ ����� ������ �Ǻ�
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
			//������ Ŭ���̾�Ʈ�� ��ȣ�� ã�Ƽ� �ش� ��ȣ�� ���� IP�� ����Ѵ�
			strcpy(ipAddress[client_num], inet_ntoa(their_addr.sin_addr));
			printf("Client IP: %s, %d\n", ipAddress[client_num], (int)strlen(ipAddress[client_num]));

			strcpy(sharedBuf, shmaddr);

			//Ŭ���̾�Ʈ�� ����� �ڽ� ������ ���ο� �б���
			pid = fork();
			if (pid > 0) {
				//�޽����� echo�ϴ� �κ�
				//�����޸𸮸� ��� ����͸��Ͽ� ��ȭ�� �����Ǹ� �޽����� Ŭ���̾�Ʈ�� ����
				while (1) {
					while (!strcmp(shmaddr, sharedBuf)) {
						usleep(100);
					}
					strcpy(sharedBuf, shmaddr);

					//�޽����� ���� Ŭ���̾�Ʈ�� �����ϰ� �޽����� ECHO
					if (shmaddr[1] != (client_num + 48)) {
						printf("%s\n", shmaddr);
						//���� ���� ��û�� �Դ��� üũ
						if (strstr(shmaddr, "FILE") != NULL) {
							//���� ���� ���� ��û�� �Դٸ� ��û�� Ŭ���̾�Ʈ�� ���ؼ� ���� ����
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
				//�޽����� �޴ºκ�
				while (1) {
					rcv_byte = recv(new_fd, buf, sizeof(buf), 0);
					if (rcv_byte <= 0)	continue;
					else	printf("%d=recv:%s\n", client_num, buf);

					//���� ���� ��û�� �Դٸ�
					if (strstr(buf, "FILE") != NULL) {
						//��û�� ������ �̸��� �˻�
						int temp = FindUser(strstr(buf, "FILE"), UserID);
						//������ ��Ͽ� ���ٸ� NOUSER�޽��� ����
						if (temp == -1) {
							send(new_fd, "NOUSER\0", 7, 0);
							continue;
						}
						//������ ��Ͽ� ������ �������� �ʾ������ NOUSER�޽��� ����
						else if (UserIn[temp] == 0) {
							send(new_fd, "NOUSER\0", 7, 0);
							continue;
						}
						//���� ���� ��û�� ���� IP�� ����
						else {
							send(new_fd, ipAddress[temp], strlen(ipAddress[temp]), 0);
						}
					}
					//�����޸𸮿� �޽��� ����
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
