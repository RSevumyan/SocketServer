#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void hup_handler(int signo);

void term_handler(int signo);

int daemonServer(void);

void getResponse(int fd, char *request, char* res);

int create_conn(int port);

int create_client(int port);

int create_unix_conn(int num);

int create_unix_client(int num);

int conffd;
int logfd;
int sock_fd, cli_fd;
char chport[4] = "7500";
int pid1, pid2, pid3;
char logpath[50];
char port[10];
static int isParent;

int main(void) {
    int pid;

    if ((pid = fork()) < 0) {
        printf("Ошибка при создании демона\n\n");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        //exit(EXIT_SUCCESS);
        setsid();
        daemonServer();
    } else {
        //setsid();
        //daemonServer();
    }
}

int daemonServer(void) {

    signal(SIGCHLD, SIG_IGN);
    isParent = 1;
    const char confpath[] = "/home/robi/workspace/CLion/Task5/Server/conf.ini";
    char buff[50], outbuff[50];
    int cli_size;

    //Задаем обработчик на SIG_HUP
    struct sigaction hup_act;
    hup_act.sa_handler = hup_handler;
    sigemptyset(&hup_act.sa_mask);
    hup_act.sa_flags = 0;
    sigaction(SIGHUP, &hup_act, NULL);

    //Задаем обработчик на SIG_TERM
    struct sigaction term_act;
    term_act.sa_handler = term_handler;
    sigemptyset(&term_act.sa_mask);
    term_act.sa_flags = 0;
    sigaction(SIGHUP, &term_act, NULL);


    //Считываем конфигурационный файл
    conffd = open(confpath, O_RDONLY);
    if (conffd < 0) {
        perror("Ошибка при открытии конфигрурационного файла\n\n");
        exit(EXIT_FAILURE);
    }
    read(conffd, buff, sizeof buff);
    strcpy(logpath, strtok(buff, "\n"));
    strcpy(port, strtok(NULL, "\n"));
    //strcpy(chport, strtok(NULL, "\n"));

    //Создаем лог
    logfd = creat(logpath, 502);
    if (logfd < 0) {
        perror("Ошибка при создании лог файла\n\n");
        exit(EXIT_FAILURE);
    }
    dup2(logfd, fileno(stdout));
    dup2(logfd, fileno(stderr));
    //close(logfd);

    printf("Создан сервисный соккет\n\n");
    //Создаем подключение
    sock_fd = create_conn(atoi(port));

    printf("Начато прослушивание\n\n");
    listen(sock_fd, 5);

    struct sockaddr_in cli_addr;

    printf("Ожидание запроса клиента\n\n");

    while (isParent) {
        cli_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, (socklen_t *) &cli_size);
        if (cli_fd > 0) {
            if (isParent && read(cli_fd, buff, sizeof(buff)) > 0) {
                printf("Принят запрос \"%s\"\n\n", buff);
                getResponse(cli_fd,buff,outbuff);
                if(isParent) {
                    write(cli_fd, outbuff, sizeof(buff));
                    printf("I'm out\n");
                    close(cli_fd);
                }
            }

        }

    }
    return 0;
}

void getResponse(int fd, char *request, char *res) {
    int num;
    char number;

    printf("Подготовка ответа\n\n");

    if ((pid1 = fork()) > 0) {
        if ((pid2 = fork()) > 0) {
            if ((pid3 = fork()) > 0) {
                //Секция родительская
                int fd1, fd2, fd3, sfd;
                struct sockaddr_in addr;

                printf("Создание сокета для общения с дочерними процессами\n\n");

                sfd = create_unix_conn(1);

                listen(sfd, 5);

                fd1 = accept(sock_fd, (struct sockaddr *) &addr, (socklen_t *) sizeof addr);
                fd2 = accept(sock_fd, (struct sockaddr *) &addr, (socklen_t *) sizeof addr);
                fd3 = accept(sock_fd, (struct sockaddr *) &addr, (socklen_t *) sizeof addr);

                printf("Отправка дочерним процессам чисел\n\n");

                for (int i = 0; i < sizeof(request); i++) {
                    switch (i % 3) {
                        case 1 :
                            write(fd1, &request[i], 1);
                            read(fd1, &res[i], 1);
                            break;
                        case 2 :
                            write(fd2, &request[i], 1);
                            read(fd2, &res[i], 1);
                            break;
                        case 0 :
                            write(fd3, &request[i], 1);
                            read(fd3, &res[i], 1);
                            break;
                        default:
                            break;
                    }
                }
                close(fd1);
                close(fd2);
                close(fd3);
                //kill(pid1, SIGKILL);
                //perror("kill1");
                //kill(pid2, SIGKILL);
                //perror("kill2");
                //kill(pid3, SIGKILL);
                //perror("kill3");
                close(sfd);
                printf("Возвращаем результат:%s\n\n", request);

                //return request;

            } else if (pid3 == 0) {
                res[0]=0;
                isParent = 0;
                //Секция дочерняя 3
                sock_fd = create_unix_client(3);

                printf("Ожидание запроса\n\n");

                if (sock_fd < 0) {
                    printf("Нет соеденения\n\n");
                } else {
                    while (read(sock_fd, &number, 1) > 0) {
                        num = atoi(&number);
                        num = num >> 1;
                        number = num + '0';
                        write(sock_fd, &number, 1);
                    }

                }
            }
        } else if (pid2 == 0) {
            //Секция дочерняя 2
            res[0]=0;
            isParent = 0;
            sock_fd = create_unix_client(2);

            printf("Ожидание запроса\n\n");

            if (sock_fd < 0) {
                printf("Нет соеденения\n\n");
            } else {
                while (read(sock_fd, &number, 1) > 0) {
                    num = atoi(&number);
                    num = num >> 1;
                    number = num + '0';
                    write(sock_fd, &number, 1);
                }

            }

        }
    } else if (pid1 == 0) {
        //Секция дочерняя 1
        res[0]=0;
        isParent = 0;
        sock_fd = create_unix_client(1);

        printf("Ожидание запроса\n\n");
        if (sock_fd < 0) {
            printf("Нет соеденения\n\n");
        } else {
            while (read(sock_fd, &number, 1) > 0) {
                num = atoi(&number);
                num = num >> 1;
                number = num + '0';
                write(sock_fd, &number, 1);
            }

        }
    }
}

int create_conn(int port) {
    int result, fd;
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons((uint16_t) port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd = socket(AF_INET, SOCK_STREAM, 0);
    result = bind(fd, (struct sockaddr *) &serv_addr, sizeof serv_addr);

    if (result < 0) {
        //printf("Ошибка создания канала c портом %d\n\n", port);
        write(1,"Ошибка создания канала c портом\n", 100);
        exit(EXIT_FAILURE);
    } else {
        printf("Создан сокет с портом %d\n\n", port);
    }

    return fd;
}

int create_unix_conn(int num) {
    int result, fd;
    char pt[100];
    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    sprintf(pt, "/home/robi/workspace/CLion/Task5/Server/sock%d", num);
    strcpy(serv_addr.sun_path, pt);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    result = bind(fd, (struct sockaddr *) &serv_addr, sizeof serv_addr);

    if (result < 0) {
        printf("Ошибка создания сокета c файлом %s\n\n", pt);
        perror("Ошибка");
        close(fd);
        exit(EXIT_FAILURE);
    } else {
        printf("Создан сокет с файлом %s\n\n", pt);
    }

    return fd;
}

int create_client(int port) {
    int fd;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    //serv_addr.sin_port = htons((uint16_t) atoi(conf[0]));

    address.sin_port = htons((uint16_t) port);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(fd, (struct sockaddr *) &address, sizeof address) < 0) {
        printf("Ошибка при подключении к соккету c портом %d\n\n", port);
        exit(EXIT_FAILURE);
    } else {
        printf("Создано подключение к соккету с портом %d\n\n", port);
    }
    return fd;
}

int create_unix_client(int num) {
    int result, fd;
    char pt[100];
    struct sockaddr_un serv_addr;
    serv_addr.sun_family = AF_UNIX;
    sprintf(pt, "/home/robi/workspace/CLion/Task5/Server/sock%d", num);
    strcpy(serv_addr.sun_path, pt);
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    result = connect(fd, (struct sockaddr *) &serv_addr, sizeof serv_addr);

    if (result < 0) {
        printf("Ошибка создания сокета c файлом %s\n\n", pt);
        exit(EXIT_FAILURE);
    } else {
        printf("Создан сокет с файлом %s\n\n", pt);
    }

    return fd;
}

void hup_handler(int signo) {

    char buf[50];
    char res1[10];
    char res2[10];
    int result;

    read(conffd, buf, sizeof buf);
    strcpy(res1, strtok(buf, ";;"));
    strcpy(res2, strtok(NULL, ";;"));

    while (strcmp(res1, logpath) || strcmp(res2, port)) {
        unlink(logpath);
        strcpy(logpath, res1);
        strcpy(port, res2);

        logfd = open(logpath, O_WRONLY);
        dup2(STDERR_FILENO, logfd);

        struct sockaddr_un address;
        address.sun_family = AF_UNIX;
        strcpy(address.sun_path, logpath);
        sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
        result = bind(sock_fd, (struct sockaddr *) &address, sizeof address);
        if (result < 0) {
            perror("Ошибка создания канала\n\n");
            exit(EXIT_FAILURE);
        }
    }
}

void term_handler(int signo) {
    kill(pid1, SIGKILL);
    printf("Дочерний процесс 1 завершен\n\n");
    kill(pid2, SIGKILL);
    printf("Дочерний процесс 2 завершен\n\n");
    kill(pid3, SIGKILL);
    printf("Дочерний процесс 3 завершен\n\n");
    close(sock_fd);
    printf("Закрыли сокет\n\n");
    exit(EXIT_SUCCESS);
}

