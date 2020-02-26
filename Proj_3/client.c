#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>  
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <dirent.h>

#define SERV_IP		"220.149.128.100"
#define SERV_PORT	4051
#define CLNT_PORT	4052
#define BACKLOG		10

int fileServe(void);
int fileGet(char* serverIP);

//파일을 보내는 쪽에서 실행하는 함수. 파일을 송신
int fileServe(void)
{
	int rcv_byte;
	int sockfd, new_fd;
	pid_t pid;
	unsigned int sin_size;
	int val = 1;

	int fileNum = 0;
	int file_len = 0;
	char fileName[20] = "./";
	FILE* file;

	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;

	char buf[30];
	//char buf2[512];
	char fileBuf[2];

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	if (sockfd == -1) {
		perror("Sercer-socket() error lol!");
		exit(1);
	}
	//else printf("Server-socket() sockfd is OK...\n");
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(CLNT_PORT);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), 0, 8);
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&val, sizeof(val)) < 0) {
		perror("setsockopt");
		close(sockfd);
	}

	if (bind(sockfd, (struct sockaddr*) & my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Server=bind() error lol!");
		exit(1);
	}
	//else printf("Server-bind() is OK...\n");

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen() error lol!");
		exit(1);
	}
	//else printf("listen() is OK...\n\n");

	sin_size = sizeof(struct sockaddr_in);
	signal(SIGCHLD, SIG_IGN);

	new_fd = accept(sockfd, (struct sockaddr*) & their_addr, &sin_size);

	printf("\n====== File Sending Process is Ready =====\n");
	printf("File List : \n");

	//파일 리스트를 출력하는 부분.
	//파일리스트를 번호를 붙여서 상대에게 보낸다.
	DIR* dir;
	int fileCount = 0;
	char files[20][15];
	struct dirent* ent;
	dir = opendir("./");
	if (dir != NULL) {

		/* print all the files and directories within directory */
		while ((ent = readdir(dir)) != NULL) {
			sprintf(files[fileCount], "%s", ent->d_name);
			if (files[fileCount][0] == '.') {
				continue;
			}
			sprintf(buf, "%d : %s", fileCount++, ent->d_name);
			//읽은 파일리스트를 한줄씩 전송
			send(new_fd, buf, strlen(buf) + 1, 0);
			printf("%s\n", buf);
			rcv_byte = recv(new_fd, buf, sizeof(buf), 0);
			//usleep(20);
		}
		closedir(dir);
	}
	else {
		/* could not open directory */
		perror("");
	}
	send(new_fd, "END\0", 4, 0);

	memset(buf, 0, 8);

	//상대로부터 전송할 파일의 번호를 수신한다
	//번호가 범위안에 있지 않으면 송신 프로세스 종료
	rcv_byte = recv(new_fd, &fileNum, sizeof(fileNum), 0);
	if (!(fileNum < fileCount && fileNum >= 0)) {
		send(new_fd, "NOERR\0", 6, 0);
		printf("\n====== File Sending Process is Failed =====\n");
		close(new_fd);
		return -1;
	}
	send(new_fd, "GO\0", 3, 0);
	printf("Sending File Name: %s\n", files[fileNum]);

	//파일 이름을 전송
	send(new_fd, files[fileNum], strlen(files[fileNum]) + 1, 0);
	strcat(fileName, files[fileNum]);
	usleep(100);

	//파일을 열고 전송을 시작한다
	file = fopen(fileName, "rb");
	while (1) {
		memset(fileBuf, 0, 8);
		file_len = fread(fileBuf, sizeof(char), sizeof(fileBuf), file);
		send(new_fd, fileBuf, strlen(fileBuf) + 1, 0);
		if (feof(file))	break;
	}
	//fseek(file, file_len, 0);
	fclose(file);

	sleep(1);
	printf("\n====== File Sending Process is Done =====\n");
	close(new_fd);
	close(sockfd);
	return 0;
}

//파일을 받는 쪽에서 실행하는 함수. 파일을 수신
int fileGet(char* serverIP)
{
	int sockfd;

	struct sockaddr_in my_addr;
	struct sockaddr_in p2p_addr;

	int rcv_byte;
	pid_t pid;
	//char buf[30];
	char buf2[30];
	char test = EOF;

	char id[20];
	char pw[20];

	FILE* file;
	int fileNum = 0;
	char fileName[20] = "./";
	char fileBuf[2];

	sleep(1);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Client-socket() error lol!");
		printf("\n====== File Receiving Process is Failed =====\n");
		return -1;
	}
	//else printf("Client-socket() sockfd is OK...\n");

	my_addr.sin_family = AF_INET;

	my_addr.sin_port = htons(CLNT_PORT);
	my_addr.sin_addr.s_addr = inet_addr(serverIP);
	memset(&(my_addr.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr*) & my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Client-connect() error lol!");
		printf("\n====== File Receiving Process is Failed =====\n");
		return -1;
	}
	//else printf("Client-connect() is OK...\n");
	printf("\n====== File Receiving Process is Ready =====\n");



	while (1) {
		//파일리스트를 수신한다
		rcv_byte = recv(sockfd, buf2, sizeof(buf2), 0);
		printf("%s\n", buf2);
		if (strstr(buf2, "END") != NULL) { break; }
		send(sockfd, "1", 1, 0);
	}

	//원하는 파일번호를 입력하고 전송한다
	printf("TYPE FILE NUMBER : ");
	getchar();
	scanf("%d", &fileNum);
	send(sockfd, &fileNum, 1, 0);

	//번호가 유효한지 아닌지 여부를 수신
	rcv_byte = recv(sockfd, buf2, sizeof(buf2), 0);
	if (!strcmp(buf2, "NOERR\0")) {
		printf("Number Error!!!\n\n");
		printf("\n====== File Receiving Process is Failed =====\n");
		close(sockfd);
		return -1;
	}

	//파일이름 수신
	rcv_byte = recv(sockfd, buf2, sizeof(buf2), 0);
	printf("Receiving File Name: %s\n", buf2);
	strcat(fileName, buf2);

	//파일 수신 시작
	file = fopen(fileName, "wb");
	usleep(100);
	while ((rcv_byte = recv(sockfd, fileBuf, sizeof(fileBuf), 0)) != 0) {
		fwrite(fileBuf, sizeof(char), rcv_byte, file);
		memset(fileBuf, 0, 8);
	}
	fclose(file);

	printf("\n====== File Receiving Process is Done =====\n");
	close(sockfd);
	return 0;
}

int main()
{
	int sockfd;

	struct sockaddr_in my_addr;
	struct sockaddr_in p2p_addr;

	int rcv_byte;
	pid_t pid;
	char buf[512];
	char buf2[512];

	char id[20];
	char pw[20];

	//프로세스 종료를 위한 공유메모리를 사용한 플래그
	int* shmaddr = mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1) {
		perror("Client-socket() error lol!");
		exit(1);
	}
	else printf("Client-socket() sockfd is OK...\n");

	my_addr.sin_family = AF_INET;

	my_addr.sin_port = htons(SERV_PORT);
	my_addr.sin_addr.s_addr = inet_addr(SERV_IP);
	memset(&(my_addr.sin_zero), 0, 8);

	if (connect(sockfd, (struct sockaddr*) & my_addr, sizeof(struct sockaddr)) == -1) {
		perror("Client-connect() error lol!");
		exit(1);
	}
	else printf("Client-connect() is OK...\n");
	*shmaddr = 0;

	//서버로 ID전송
	printf("ID: ");
	scanf("%s", id);
	send(sockfd, id, strlen(id) + 1, 0);

	//서버로 PW전송
	printf("PW: ");
	scanf("%s", pw);
	send(sockfd, pw, strlen(pw) + 1, 0);

	//수신받은 메시지의 첫 머리가 1일경우 로그인 성공, 0일경우 실패
	rcv_byte = recv(sockfd, buf2, sizeof(buf2), 0);
	printf("%s\n", buf2 + 1);
	if (buf2[0] == '1') {
		//프로세스 분기
		pid = fork();
		if (pid > 0) {
			while (1) {
				//항상 SCAN을 통해 메시지 전송을 준비
				if (*shmaddr == 0) {
					memset(buf, 0, sizeof(char));
					printf(">>");
					getchar();
					scanf("%[^\n]", buf);
					//만약 FILE 요청일 경우 플래그 1로 설정
					if (strstr(buf, "FILE") != NULL) {
						*shmaddr = 1;
					}
					send(sockfd, buf, strlen(buf) + 1, 0);
					if (!strcmp(buf, "exit")) {
						//sleep();
						//break;
					}
				}
				else if (*shmaddr == 1) {

				}
				else if (*shmaddr == 2) {
					usleep(100);
					break;
				}
			}
		}
		else if (pid == 0) {
			while (1) {
				//항상 SCAN을 통해 메시지 전송을 준비
				memset(buf2, 0, sizeof(char));
				rcv_byte = recv(sockfd, buf2, sizeof(buf2), 0);
				if (*shmaddr == 1) {
					//파일 전송을 요청했으나 해당 유저가 없을 경우
					if (!strcmp(buf2, "NOUSER\0")) {
						printf("There is No User\n");
					}
					else {
						//파일 전송을 요청하여 상대 IP를 수신한다
						buf2[15] = '\0';
						//파일 수신 함수 호출
						fileGet(buf2);
						memset(buf2, 0, sizeof(char));
					}
					*shmaddr = 0;
				}
				else {
					printf("\r%s\n", buf2);
					printf(">>");
				}
				//서버로부터 파일요청의 메시지를 받음
				if (strstr(buf2, "FILE") != NULL) {
					*shmaddr = 1;
					//파일 송신 함수 호출
					fileServe();
					*shmaddr = 0;
				}
				if (!strcmp(buf2, "Bye\0")) {
					*shmaddr = 2;
					printf("\r\nType Anykey To Exit...\n\n");
					break;
				}
			}
		}
	}
	close(sockfd);
	return 0;
}
