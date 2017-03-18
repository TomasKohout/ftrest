#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>    //strlen
#include <arpa/inet.h> //inet_addr
#include <netdb.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <time.h>
#include <cstdio>
#include <stdlib.h>
#include <string>
#include <sstream>

#define PUT 1
#define DEL 2
#define GET 3
#define LST 4
#define MKD 5
#define RMD 6

#define OK 0
#define ERR 1

#define FI 0
#define FO 1
#define NOTH 2

using namespace std;

string arr[] = { "PUT", "DEL", "GET", "LST", "MKD", "RMD"};
vector<string> commands (arr, arr + 6) ;

struct arg {
    string portNum;
    string type;
    string ipAddr;
    string localPath;
    string remotePath;
    int alpha, j;
};

void errMsg(const char *msg)
{
    perror(msg);
}

int fileOrFolder(string path)
{
    struct stat s;

    if( stat(path.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
        {
            return FO;
            //typeOfMsg = "?type=folder HTTP/1.1";
            // cout << localPath + "dir" << endl;
        }
        else if( s.st_mode & S_IFREG )
        {
            return FI;
        } else
            return NOTH;

    } else
        return NOTH;
}
arg* getParams(int argc, char *argv[])
{
    char temp[PATH_MAX];
    arg * str = new arg;
    if (argc != 4 && argc != 3)
    {
        errMsg("ERROR: spatne argumetny");
        exit(ERR);
    }

    locale l;
    string arg(argv[1]);

    str->alpha = 42;
    str->j = 0;

    for (string::size_type i = 0; i < arg.length(); i++ )
        arg[i] = toupper(arg[i], l);

    while (str->j < 6 && str->alpha != 0) {
        str->alpha = arg.compare(commands[str->j]);
        str->j++;
    }
    if (arg == "PUT" && argc != 4)
    {
       // cout << argc << endl;
        delete str;
        errMsg("ERROR: Put musi obsahovat localpath");
        exit(ERR);
    }

    str->localPath = getcwd(temp, PATH_MAX);
    string temporary;

    if (argc == 4)
    {
        temporary = argv[3];
        string::size_type i = 0;
        if (temporary[i] == '~') {
           // cout << temporary << endl;
            temporary.erase(0, 1);
            str->localPath.append(temporary);
        }
        else str->localPath = temporary;

        //cout << str->localPath << endl;
    }
    string tmp;
    string argv2(argv[2]);

    for (unsigned i = 0; i < argv2.length(); i++) {
        if ( (argv[2][i] == ':' && tmp.compare("http://") == 0) || (argv[2][i] == '/' && tmp.compare(":") == 0)) {
            if (tmp.compare(":") == 0) {
                tmp = "";
                tmp += "/";
                i--;
            }
            else{
                tmp =   "";
                tmp += ":";
            }
        }
        else if (tmp.compare("http://") == 0 )
        {
            if (argv[2][i] != '/')
                str->ipAddr += argv[2][i];
            else
            {
                tmp = "/";
                i--;
            }
        }
        else if ( tmp.compare(":") == 0)
            str->portNum += argv[2][i];
        else if (tmp.compare("/") == 0)
        {
            if (isspace(argv[2][i]))
                str->remotePath += "%20";
            else
                str->remotePath += argv[2][i];
        }
        else
            tmp += argv[2][i];
    }

    if (str->portNum.empty())
        str->portNum = "6677";
    if (str->ipAddr.empty() || str->portNum.empty() || str->remotePath.empty() )
    {
        delete str;
        errMsg("Spatne zadane parametry");
        exit(ERR);
    }

    return str;
}

string getFirstLineOfResponse(string rest)
{
    string ret;
    string::size_type i = 0;
    while(rest[i] != '\r')
    {
        ret += rest[i];
        i++;
    }
    ret += "\n";

    return ret;
}

string getHeader(string rest)
{
    string control = getFirstLineOfResponse(rest);

    string::size_type i = 3;

    while(!(rest[i-3] == '\r' && rest[i-2] == '\n' && rest[i-1] == '\r' && rest[i] == '\n'))
        i++;
    rest.erase(0, i+1);

    return rest;
}
string createHeader(int type, string path)
{
    char buf[1024];
    string rest;
    time_t now = time(0);
    tm tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);
    switch (type)
    {
        case DEL:
            rest = "DELETE ";
            rest.append(path);
            rest += "?type=file HTTP/1.1";
            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: text/plain\r\nAccept-Encoding: identity\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";

            break;

        case RMD:
            rest = "DELETE ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";
            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: text/plain\r\nAccept-Encoding: identity\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";

            break;
        case GET:
            rest = "GET ";
            rest.append(path);
            rest += "?type=file HTTP/1.1";
            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: application/octet-stream\r\nAccept-Encoding: identity\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";
            break;
        case LST:
            rest = "GET ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";

            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: text/plain\r\nAccept-Encoding: identity\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";


            break;
        case PUT:
            rest = "PUT ";
            rest.append(path);
            rest += "?type=file HTTP/1.1";
            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: text/plain\r\nAccept-Encoding: identity\r\nContent-Type: ";
            break;

        case MKD:
            rest = "PUT ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";
            rest += "\r\nDate: ";
            rest.append(buf);
            rest += "\r\nAccept: text/plain\r\nAccept-Encoding: identity\r\nContent-Type: text/plain\r\nContent-Length: 0\r\n\r\n";

            break;
    }

    return rest;
}

string readSock(int socket){

    char buff[1024];
    string rcv;
    int n = 0;
    while(42) {
        bzero(buff, 1024);
        n = recv(socket, buff, 1024,0);
        if (n > 0) {
            rcv += string(buff,n);
        }
        else if (n == 0) {
            break;
        } else {
            fprintf(stderr, "UNKNOWN ERROR\n");
            exit(ERR);
        }
    }
    return rcv;
}

int writeSock(int socket, string buf)
{
    long size = buf.size();

    while (size > 0)
    {
        int n = (int) send(socket, (void*)buf.data(), buf.size(),0);
        shutdown(socket, SHUT_WR);
        if (n <= 0) {
            if (n == 0) {
                errMsg("ERROR: disconnected");
                return ERR;
            }
            else
            {
                errMsg("ERROR: nic nebylo zapsano");
                return ERR;
            }
        }
        size -= n;
    }

    return OK;
}

string getMIME(string localPath)
{
    FILE *mime;
    char b[200];
    string mi;
    string a = "file -b --mime-type " + localPath;
    if ((mime = popen(a.c_str(), "r")) == NULL)
    {
        errMsg("ERROR: popen neziskal mime, typ bude: application/octet-stream");
        pclose(mime);

        mi = "application/octet-stream";
        return mi;
    }
    while(fgets(b, 200, mime))
        mi += b;
    pclose(mime);
    return mi;
}



int put (int sock, string path, int type, string localPath)
{
    FILE *file;
    int n;
    int k;
    long fsize;
    char buf[1024];
    char *content = NULL;
    string rest;
    int err;
    string typeOfMsg;
    long save;


    if (type == PUT) {


        rest = createHeader(type, path);

        if (fileOrFolder(localPath) != FI)
        {
            errMsg("ERROR: nelze otevrit nic jineho nez soubor");
            return ERR;
        }

        file = fopen(localPath.c_str(), "rb");
        if (!file) {
            errMsg("ERROR: pri otevirani souboru");
            return ERR;
        }
        if (fseek(file, 0, SEEK_END) == -1) {
            errMsg("ERROR: nelze dojit na konec souboru");
            return ERR;
        }
        fsize = ftell(file);
        if (fsize == -1) {
            errMsg("ERROR: zadna velikost souboru");
            return ERR;
        }


        stringstream ss;
        ss << fsize;
        string str = ss.str();
        rest += getMIME(localPath);

        rewind(file);

        content = (char *) malloc(fsize);
        if (!content) {
            errMsg("ERROR: nenaalokoval se prostor pro file");
            return ERR;
        }

        if (fread(content, fsize, 1, file) != 1)
        {
            errMsg("ERROR: soubor nebyl nacten");
            free(content);
            fclose(file);
            return ERR;
        }
        fclose(file);
        rest += "\rContent-Length: ";
        rest += str + "\r\n\r\n";
        rest += string(content,fsize);
        free(content);

        writeSock(sock, rest);

        rest = readSock(sock);

        if (getFirstLineOfResponse(rest) != "HTTP/1.1 200 OK\n")
        {
            cerr << getHeader(rest);
            return ERR;
        }



        //if (rest != "HTTP/1.1 200 OK\n")
          //  cerr << readSock(sock);
    }
    else if (type == MKD)
    {
        rest = createHeader(type, path);
        writeSock(sock, rest);
        rest = readSock(sock);

        if (getFirstLineOfResponse(rest) != "HTTP/1.1 200 OK\n")
        {
            cerr << getHeader(rest);
            return ERR;
        }
    }

    return OK;
}
int get(int sock, string path, int type, string localPath)
{
    string err;
    FILE *file;
    string rest;

    if (type == GET)
    {
        //size_t f = path.find_last_of("/");
        //localPath += path.substr(f);

        rest = createHeader(type, path);

        writeSock(sock,rest);

        rest = readSock(sock);
        if (getFirstLineOfResponse(rest) != "HTTP/1.1 200 OK\n")
        {
            cerr << getHeader(rest);
            return ERR;
        }

        rest = getHeader(rest);

        file = fopen(localPath.c_str(), "wb");

        if (file == NULL )
        {
            errMsg("ERROR: could not open the path.");
            return ERR;
        } //todo error handling to the client

        fwrite(rest.c_str(),1,rest.size(), file);
        //todo does file exists?


        fclose(file);

    }
    else if ( type == LST)
    {
        rest = createHeader(type, path);

        writeSock(sock, rest);


        rest = readSock(sock);
        err = getFirstLineOfResponse(rest);
        if (err == "HTTP/1.1 200 OK\n") {
            cout << getHeader(rest);
        }
        else {
            cerr << getHeader(rest);
            return ERR;
        }
    }
    return OK;

}

int del(int sock, string path, int type)
{
    string rest,err;
    rest = createHeader(type, path);
    // cout << rest << endl;;
    writeSock(sock, rest);
    rest = readSock(sock);
    err  = getFirstLineOfResponse(rest);

    if (err != "HTTP/1.1 200 OK\n")
    {
        cerr << getHeader(rest);
        return ERR;
    }

    return OK;
}
int main(int argc , char *argv[])
{

    arg *str = getParams(argc, argv);

   // cout << str->ipAddr;
    int sockfd, portno, n;
    sockaddr_in serv_addr;
    hostent *server = NULL;

    portno = atoi(str->portNum.c_str());
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (portno > 65535 && portno < 0)
    {
        errMsg( "Porty jsou v rozsahu 0 - 65535. Doporučuji používat porty nad 1024.");
        delete str;
        return ERR;
    }

    if (sockfd < 0)
    {
        errMsg( "Nevytvořil se socket");
        delete str;
        return ERR;
    }

    server = gethostbyname(str->ipAddr.c_str());

    if (server == NULL)
    {
        errMsg("Nevytvořil hosta");
        delete str;
        return ERR;
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family    =   AF_INET;
    bcopy((char *) server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port  =   htons(portno);
    n = (int) connect(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr));
    if (n < 0)
    {
        errMsg("ERROR: connecting");
        delete str;
        return ERR;
    }

    if (str->alpha == 0)
    {   // cout << commands[PUT-1].c_str() << endl;
        switch (str->j)
        {
            case PUT:
                n = put(sockfd, str->remotePath, PUT, str->localPath);
                break;
            case DEL:
                n = del(sockfd, str->remotePath, DEL);
                break;
            case GET:
                n = get(sockfd, str->remotePath, GET , str->localPath);
                break;
            case LST:
                n = get(sockfd, str->remotePath, LST , str->localPath);
                break;
            case MKD:
                n = put(sockfd, str->remotePath, MKD, str->localPath);
                break;
            case RMD:
                n = del(sockfd, str->remotePath, RMD);
                break;
            default:
                cerr << "Hello Chuck! I've been waiting for you" << endl;
        }
    }
    else
    {
        delete str;
        close(sockfd);
        errMsg("ERROR: Neznamy parametr");
        return ERR;
    }
    close(sockfd);
    delete str;

    if (n == OK)
        return 0;
    else
    {
        return ERR;
    }
}