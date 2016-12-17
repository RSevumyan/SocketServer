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
#include <wait.h>

void hup_handler(int signo);

void term_handler(int signo);

int daemonServer(void);

char *response(int fd, char *request);

int create_conn(int port);

int create_client(int port);

int conffd;
int logfd;
int sock_fd, cli_fd;
int pid1, pid2, pid3;
char logpath[50];
char port[10];

int main(void) {
    int pid;

    if ((pid = (int) fork) < 0) {
        printf("Ошибка при создании демона\n");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        exit(EXIT_SUCCESS);
    } else {
        daemonServer();
        exit(0);
    }
}

int daemonServer(void) {

    const char confpath[] = "/home/robi/workspace/CLion/Task5/Server/conf.ini";
    char buff[50];
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
        perror("Ошибка при открытии конфигрурационного файла\n");
        exit(EXIT_FAILURE);
    }
    read(conffd, buff, sizeof buff);
    strcpy(logpath, strtok(buff, "\n"));
    strcpy(port, strtok(NULL, "\n"));

    //Создаем лог
    logfd = creat(logpath, 502);
    if (logfd < 0) {
        perror("Ошибка при создании лог файла\n");
        exit(EXIT_FAILURE);
    }
    int i = dup(logfd);
    dup2(logfd, STDOUT_FILENO);
    dup2(logfd, STDERR_FILENO);

    //Создаем подключение
    sock_fd = create_conn(atoi(port));

    listen(sock_fd, 5);

    struct sockaddr_in cli_addr;

    puts("Ожидание запроса\n");
    while (1) {
        cli_fd = accept(sock_fd, (struct sockaddr *) &cli_addr, (socklen_t *) &cli_size);
        if (cli_fd < 0) {
        } else {
            while (read(cli_fd, buff, sizeof(buff)) > 0) {
                write(cli_fd, response(cli_fd, buff), sizeof(buff));
            }
            close(cli_fd);
        }
    }
}

char *response(int fd, char *request) {
    int num;
    char number;

    if (pid1 = fork() > 0) {
        if (pid2 = fork() > 0) {
            if (pid3 = fork() > 0) {
                //Секция родительская
                sleep(1);
                int fd1, fd2, fd3;
                
                fd1 = create_client(9001);
                fd2 = create_client(9002);
                fd3 = create_client(9003);
                for (int i = 0; i < sizeof(request); i++) {
                    switch (i % 3) {
                        case 1 :
                            write(fd1, &request[i], 1);
                            read(fd1, &request[i], 1);
                            break;
                        case 2 :
                            write(fd2, &request[i], 1);
                            read(fd2, &request[i], 1);
                            break;
                        case 0 :
                            write(fd3, &request[i], 1);
                            read(fd3, &request[i], 1);
                            break;
                        default:
                            break;
                    }
                }
                close(fd1);
                close(fd2);
                close(fd3);

            } else if (pid3 == 0) {
                //Секция дочерняя 3
                struct sockaddr_in parent_addr;
                int size;

                sock_fd = create_conn(9003);

                listen(sock_fd, 5);

                puts("Ожидание запроса\n");
                while (1) {
                    cli_fd = accept(sock_fd, (struct sockaddr *) &parent_addr, (socklen_t *) &size);
                    if (cli_fd < 0) {
                        puts("Нет соеденения\n");
                    } else {
                        while (read(cli_fd, &number, 1) > 0) {
                            num = atoi(&number);
                            num = num >> 1;
                            number = num + '0';
                            write(cli_fd, &number, 1);
                        }
                        close(cli_fd);
                        return 0;
                    }
                }
            }
        } else if (pid2 == 0) {
            //Секция дочерняя 2
            struct sockaddr_in parent_addr;
            int size;

            sock_fd = create_conn(9002);

            listen(sock_fd, 5);

            puts("Ожидание запроса\n");
            while (1) {
                cli_fd = accept(sock_fd, (struct sockaddr *) &parent_addr, (socklen_t *) &size);
                if (cli_fd < 0) {
                    puts("Нет соеденения\n");
                } else {
                    while (read(cli_fd, &number, 1) > 0) {
                        num = atoi(&number);
                        num = num >> 1;
                        number = num + '0';
                        write(cli_fd, &number, 1);
                    }
                    close(cli_fd);
                    return 0;
                }
            }
        }
    } else if (pid1 == 0) {
        //Секция дочерняя 1
        struct sockaddr_in parent_addr;
        int size;

        sock_fd = create_conn(9001);

        listen(sock_fd, 5);

        puts("Ожидание запроса\n");
        while (1) {
            cli_fd = accept(sock_fd, (struct sockaddr *) &parent_addr, (socklen_t *) &size);
            if (cli_fd < 0) {
                puts("Нет соеденения\n");
            } else {
                while (read(cli_fd, &number, 1) > 0) {
                    num = atoi(&number);
                    num = num >> 1;
                    number = num + '0';
                    write(cli_fd, &number, 1);
                }
                close(cli_fd);
                return 0;
            }
        }
    }
    return request;
}

int create_conn(int port) {
    int result;
    struct sockaddr_in serv_addr, cli_addr;
    serv_addr.sin_family = AF_INET;

    serv_addr.sin_port = htons((uint16_t) port);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    result = bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof serv_addr);

    if (result < 0) {
        printf("Ошибка создания канала c портом %d\n", port);
        exit(EXIT_FAILURE);
    }

    return sock_fd;
}

int create_client(int port) {
    int result;
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    //serv_addr.sin_port = htons((uint16_t) atoi(conf[0]));

    address.sin_port = htons((uint16_t) port);
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (connect(sock_fd, (struct sockaddr *) &address, sizeof address) < 0) {
        printf("Ошибка при подключении к соккету c портом %d\n", port);
        exit(EXIT_FAILURE);
    }
    return sock_fd;
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
            perror("Ошибка создания канала\n");
            exit(EXIT_FAILURE);
        }

    }

}

void term_handler(int signo) {
    kill(pid1, SIGKILL);
    printf("Дочерний процесс 1 завершен");
    kill(pid2, SIGKILL);
    printf("Дочерний процесс 2 завершен");
    kill(pid3, SIGKILL);
    printf("Дочерний процесс 3 завершен");
    close(sock_fd);
    printf("Закрыли сокет");
}

