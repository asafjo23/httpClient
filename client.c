#include <assert.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

typedef enum
{
    false,
    true
} bool;

#define ERROR -1
#define BUFFER 50
#define BUFFER_SIZE 1024
#define h_addr h_addr_list[0]

/*Struct to hold all the command line arguments together*/
typedef struct request
{
    char *url;         //make copy of the url
    char *hostName;    //parse url to host name
    char *port;        //hold port
    char *path;        //hold path
    char *body;        //hold body if -p flag is up
    char **arguments;  //catch all the desired arguments
    int contentLength; //the body length for the header
    int argumentNum;   //hold the number of the arguments
    int length;        //total length for the header
} Request;

Request *create_request() // header constructor
{
    Request *request = (Request *)malloc(sizeof(Request));
    assert(request != NULL);
    request->url = NULL;
    request->arguments = NULL;
    request->body = NULL;
    request->hostName = NULL;
    request->path = NULL;
    request->port = NULL;
    request->contentLength = 0;
    request->argumentNum = 0;
    request->length = BUFFER;
    return request;
}

void free_request(Request *request) // free heep memory allocation
{
    if (request->url != NULL)
        free(request->url);
    if (request->arguments != NULL)
    {
        for (int i = 0; i < request->argumentNum; i++)
        {
            if (request->arguments[i] != NULL)
                free(request->arguments[i]);
        }
        free(request->arguments);
    }

    if (request->body != NULL)
        free(request->body);
    free(request);
}

void usage_message() //Print out the required input
{
    printf("Usage: client [-p] [-r n <pr1=value1 pr2=value2 ...>] <URL>\n");
}

bool validation(char *ptr) //validation function for the parsing
{
    if (strchr(ptr, '=') == NULL || ptr[0] == '=' || ptr[strlen(ptr) - 1] == '=')
        return false;
    return true;
}

bool argv_validation(int argc, char **argv) //validation function for the parsing
{
    for (int i = 1; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            if (strcmp(argv[i], "-r") != 0 && strcmp(argv[i], "-p") != 0 && strstr(argv[i], "http://") == NULL)
            {
                usage_message();
                return false;
            }
        }
    }
    return true;
}

bool is_digit(char *check)
{
    for (int i = 0; i < strlen(check); i++)
    {
        if (check[i] != '/')
        {
            if (check[i] < '0' || check[i] > '9')
                return false;
        }
        else if(check[i] == '/')
            break;
    }
    return true;
}

/*parsing the arguments given after the -r flag, it will return ERROR[-1] if it failed*/
int parse_arguments(int argc, char **argv, Request *request)
{
    int index = 0, argumentsIndex = 0, counter = 0;
    for (int i = 0; i < argc; i++) // searching for the -r flag
    {
        if (argv[i] != NULL)
        {
            if (strcmp(argv[i], "-r") == 0)
            {
                index = i;
                break;
            }
        }
    }
    if (index == 0)
        return !ERROR;
    if (index == argc - 1 && strcmp(argv[index], "-r") == 0)
    {
        usage_message();
        return ERROR;
    }
    index++;
    if (strcmp(argv[index], "0") == 0) // determine if the number of arguments is identical 0
    {
        argv[index] = NULL;
        return !ERROR;
    }
    int numOfArguments = atoi(argv[index]);
    if (numOfArguments == 0 || is_digit(argv[index]) == false) // determine if the number of arguments is a number
    {
        usage_message();
        return ERROR;
    }
    argv[index] = NULL;
    request->argumentNum = numOfArguments;
    request->arguments = (char **)malloc(numOfArguments * sizeof(char *));
    memset(request->arguments, 0, numOfArguments * sizeof(char *));
    assert(request->arguments != NULL);
    for (int i = index + 1; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            if (argv[i - 1] != NULL)
            {
                if (strcmp(argv[i - 1], "-p") != 0)
                {
                    if (strcmp(argv[i], "-r") == 0)
                    {
                        usage_message();
                        return ERROR;
                    }
                }
            }
            if (strcmp(argv[i], "-r") == 0 && request->arguments != NULL)
            {
                usage_message();
                return ERROR;
            }
        }
    }
    if (index + numOfArguments >= argc) // determine if the number of arguments is larger then the argv length
    {
        usage_message();
        return ERROR;
    }
    for (int i = index; i <= index + numOfArguments; i++)
    {
        if (argv[i] != NULL)
        {
            if (validation(argv[i]) == true)
            {
                request->arguments[argumentsIndex] = (char *)malloc(strlen(argv[i]) * sizeof(char) + 1);
                assert(request->arguments[argumentsIndex] != NULL);
                memset(request->arguments[argumentsIndex], 0, strlen(argv[i]));
                request->arguments[argumentsIndex][strlen(argv[i])] = '\0';
                strcpy(request->arguments[argumentsIndex], argv[i]);
                argumentsIndex++;
                counter++;
                request->length += strlen(argv[i]) + 1;
                argv[i] = NULL;
            }
        }
    }
    index = index + numOfArguments + 1;
    for (int i = index; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            if (strcmp(argv[i], "-p") != 0 && strstr(argv[i], "http://") == NULL)
            {
                usage_message();
                return ERROR;
            }
            else if (validation(argv[i]) == true)
                counter++;
        }
    }
    if (counter != numOfArguments)
    {
        if (counter > numOfArguments)
        {
            usage_message();
            return ERROR;
        }
        usage_message();
        return ERROR;
    }
    return !ERROR;
}

/*parse body, if it failed it return ERROR[-1]*/
int parse_body(int argc, char **argv, Request *request)
{
    int index = 0;
    for (int i = 0; i < argc; i++) // searching for the -p flag
    {
        if (argv[i] != NULL)
        {
            if (strcmp(argv[i], "-p") == 0)
            {
                index = i;
                break;
            }
        }
    }
    if (index == 0)
        return !ERROR;
    if (index == argc - 1 && strcmp(argv[index], "-p") == 0)
    {
        usage_message();
        return ERROR;
    }

    index++;
    request->body = (char *)malloc(strlen(argv[index]) * sizeof(char) + 1);
    for (int i = index; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            if (i >= index + 1)
            {
                if (strcmp(argv[i], "-p") == 0)
                {
                    usage_message();
                    return ERROR;
                }
            }
        }
    }
    assert(request->body != NULL);
    strcpy(request->body, argv[index]);
    request->body[strlen(argv[index])] = '\0';
    request->contentLength = strlen(argv[index]);
    request->length += request->contentLength;
    argv[index] = NULL;
    return !ERROR;
}

/*parse url, if it failed it return ERROR[-1]*/
int parse_url(int argc, char **argv, Request *request)
{
    char *url = NULL;
    int index = 0;
    for (int i = 0; i < argc; i++) // searching the URL
    {
        if (argv[i] != NULL)
        {
            if (strstr(argv[i], "http://") != NULL)
            {
                url = argv[i];
                index = i;
                break;
            }
        }
    }
    index++;

    for (int i = index; i < argc; i++)
    {
        if (argv[i] != NULL)
        {
            if (argv[i - 1] != NULL)
            {
                if (strcmp(argv[i - 1], "-p") != 0)
                {
                    if (strstr(argv[i], "http://") != NULL && url != NULL)
                    {
                        usage_message();
                        return ERROR;
                    }
                }
            }
            if (strstr(argv[i], "http://") != NULL && url != NULL)
            {
                usage_message();
                return ERROR;
            }
            else if (strcmp(argv[i], "-p") != 0 && strcmp(argv[i], "-r") != 0 && request->arguments == NULL && request->body == NULL)
            {
                usage_message();
                return ERROR;
            }
        }
    }

    if (url == NULL)
    {
        usage_message();
        return ERROR;
    }
    request->url = (char *)malloc(strlen(url) * sizeof(char) + 1);
    strcpy(request->url, url);
    request->url[strlen(url)] = '\0';
    request->length += strlen(request->url);
    char *ptr = strchr(request->url, '/') + 2;
    request->hostName = ptr;
    ptr = strchr(ptr, ':');
    if (ptr != NULL)
    {
        *ptr = '\0';
        ++ptr;
        if (*ptr == '\0' || *ptr == '/' || is_digit(ptr) == false)
        {
            usage_message();
            return ERROR;
        }
        request->port = ptr;
        request->length += strlen(request->port) + 1;
    }
    else
    {
        ptr = request->hostName;
    }
    ptr = strchr(ptr, '/');
    if (ptr != NULL)
    {
        *ptr = '\0';
        request->path = ++ptr;
        request->length += strlen(request->path) + 1;
    }
    return !ERROR;
}

/*connecting the http header together, if it failed it return NULL*/
char *create_http(Request *request)
{
    char *posix = (char *)malloc(request->length * sizeof(char));
    assert(posix != NULL);
    memset(posix, 0, request->length);
    if (posix == NULL)
    {
        perror("malloc");
        return NULL;
    }
    if (request->body != NULL)
        strcat(posix, "POST /");
    else
        strcat(posix, "GET /");
    if (request->path != NULL)
        strcat(posix, request->path);
    if (request->arguments != NULL)
    {
        strcat(posix, "?");
        for (int i = 0; i < request->argumentNum; i++)
        {
            if (request->arguments[i] != NULL)
            {
                strcat(posix, request->arguments[i]);
                if (i != request->argumentNum - 1)
                    strcat(posix, "&");
            }
        }
    }
    strcat(posix, " HTTP/1.0\r\nHost: ");
    strcat(posix, request->hostName);
    if (request->port != NULL)
    {
        strcat(posix, ":");
        strcat(posix, request->port);
    }
    if (request->contentLength > 0)
    {
        strcat(posix, "\r\nContent-length:");
        char buffer[BUFFER];
        sprintf(buffer, "%d", request->contentLength);
        strcat(posix, buffer);
    }
    strcat(posix, "\r\n\r\n");
    if (request->body != NULL)
        strcat(posix, request->body);
    return posix;
}

/*putting the socket up, return ERROR on failure*/
int make_socket(Request *request, char *posix)
{
    struct hostent *hp;
    struct sockaddr_in addr;
    int live = 1, sock;
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    if ((hp = gethostbyname(request->hostName)) == NULL) //converting host name to ip
    {
        herror("gethostbyname");
        return ERROR;
    }
    memcpy(&addr.sin_addr, hp->h_addr, hp->h_length); //coping the host to the struct
    if (request->port != NULL)                        //if there is diffrent port then 80
        addr.sin_port = htons(atoi(request->port));
    else
        addr.sin_port = htons(80);
    addr.sin_family = AF_INET;
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (const char *)&live, sizeof(int));
    if (sock == -1)
    {
        perror("setsockopt");
        return ERROR;
    }
    if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        return ERROR;
    }
    int sum = 0, nbytes = 0;
    while (true)
    {
        int nbytes = write(sock, posix, strlen(posix));
        sum += nbytes;
        if (sum == strlen(posix))
            break;
        if (nbytes < 0)
        {
            perror("write");
            return ERROR;
        }
    }
    sum = 0;
    while ((nbytes = read(sock, buffer, BUFFER_SIZE - 1)) != 0)
    {
        sum += nbytes;
        fprintf(stderr, "%s", buffer);
        bzero(buffer, BUFFER_SIZE);
        if (sum <= 0)
        {
            perror("read");
            return ERROR;
        }
    }
    printf("\nTotal received response bytes: %d\n", sum);
    shutdown(sock, SHUT_RDWR);
    close(sock);
    return !ERROR;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        usage_message();
        return EXIT_FAILURE;
    }
    Request *request = create_request();
    int iterate = parse_body(argc, argv, request);
    if (iterate == ERROR)
    {
        free_request(request);
        return EXIT_FAILURE;
    }

    iterate = parse_arguments(argc, argv, request);
    if (iterate == ERROR)
    {
        free_request(request);
        return EXIT_FAILURE;
    }

    iterate = parse_url(argc, argv, request);
    if (iterate == ERROR)
    {
        free_request(request);
        return EXIT_FAILURE;
    }

    if (argv_validation(argc, argv) == false)
    {
        free_request(request);
        return EXIT_FAILURE;
    }
    char *posix = create_http(request);
    if (posix == NULL)
    {
        free_request(request);
        return EXIT_FAILURE;
    }
    printf("Request:\n");
    printf("%s\n", posix);
    iterate = make_socket(request, posix);
    if (iterate == ERROR)
    {
        free(posix);
        free_request(request);
        return EXIT_FAILURE;
    }
    free(posix);
    free_request(request);
    return EXIT_SUCCESS;
}