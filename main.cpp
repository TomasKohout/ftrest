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
        delete str;
        errMsg("ERROR: Put musi obsahovat localpath");
        exit(ERR);
    }

    str->localPath = getcwd(temp, PATH_MAX);
    string temporary;
    if (argc == 4)
    {
        temporary = argv[3];
        if (temporary.find("~") == 0) {
            temporary.erase(0, 1);
            str->localPath.append(temporary);
        }
        else str->localPath = temporary;
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

    return ret;
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
            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: text/plain\n\rAccept-Encoding: identity\n\rContent-Type: text/plain\n\rContent-Length: 0\n\r";

            break;

        case RMD:
            rest = "DELETE ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";
            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: text/plain\n\rAccept-Encoding: identity\n\rContent-Type: text/plain\n\rContent-Length: 0\n\r";

            break;
        case GET:
            rest = "GET ";
            rest.append(path);
            rest += "?type=file HTTP/1.1";
            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: application/octet-stream\n\rAccept-Encoding: identity\n\rContent-Type: text/plain\n\rContent-Length: 0\n\r";
            break;
        case LST:
            rest = "GET ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";

            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: text/plain\n\rAccept-Encoding: identity\n\rContent-Type: text/plain\n\rContent-Length: 0\n\r";


            break;
        case PUT:
            rest = "PUT ";
            rest.append(path);
            rest += "?type=file HTTP/1.1";
            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: text/plain\n\rAccept-Encoding: identity\n\rContent-Type: ";
            break;

        case MKD:
            rest = "PUT ";
            rest.append(path);
            rest += "?type=folder HTTP/1.1";
            rest += "\n\rDate: ";
            rest.append(buf);
            rest += "\n\rAccept: text/plain\n\rAccept-Encoding: identity\n\rContent-Type: text/plain\n\rContent-Length: 0\n\r";

            break;
    }

    return rest;
}

string readSock(int socket){

    char buf[1024];
    bzero(buf, 1024);
    int n;
    string rest;
    n = (int) read(socket, buf, 1024);

    rest = buf;
    if (rest == "Unknown error\n")
    {
        cerr << "Unknown error has occured on the server side" <<endl;
    }
    return rest;
}

int writeSock(int socket, string buf)
{
    int n = (int) write(socket, buf.c_str(), 1024);
    if (n < 0) return ERR;
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
    long fsize;
    char buf[1024];
    char *content = NULL;
    string rest;
    string typeOfMsg;
    long save;
    rest = createHeader(type, path);

    if (type == PUT) {
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
        rest += "\rContent-Length: ";
        rest += str + "\n\r";
        rewind(file);

        content = (char *) malloc((size_t) fsize);
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

       writeSock(sock, rest);

        while (fsize > 0) {
            int sent = (int) send(sock, content, fsize, 0);
            if (sent <= 0) {
                if (sent == 0) {
                    errMsg("ERROR: disconnected");
                    //free(content-save);
                    return ERR;
                }
                else
                {
                    errMsg("ERROR: nic nebylo zapsano");
                    //free(content-save);
                    return ERR;
                }
            }
            content += sent;
            fsize -= sent;
            save +=sent;
        }

        //free(content-save);
        rest = getFirstLineOfResponse(readSock(sock));

        if (rest != "HTTP/1.1 200 OK\n")
            cerr << readSock(sock);
    }
    else if (type == MKD)
    {
        writeSock(sock, rest) != OK;

        rest = getFirstLineOfResponse(readSock(sock));

        if (rest != "HTTP/1.1 200 OK\n")
            cerr << readSock(sock);
    }
}
int get(int sock, string path, int type, string localPath)
{
    char buf[1024];
    int size = 0;
    int n;
    FILE *file;
    string rest, errorControl;

    if (type == GET)
    {
        size_t f = path.find_last_of("/");
        localPath += path.substr(f);

        rest = createHeader(type, path);

        writeSock(sock,rest);

        rest = readSock(sock);
        errorControl = getFirstLineOfResponse(rest);
        if (errorControl == "HTTP/1.1 404 Not Found" || errorControl == "HTTP/1.1 400 Bad Request") {
            cerr << readSock(sock) ;
            return ERR;
        }

        file = fopen(localPath.c_str(), "wb");
        if (file == NULL )
        {
            errMsg("ERROR: could not open the path.");
            return ERR;
        } //todo error handling to the client

        string tmp = "";
        string sizes = "";

        int cout = 0;
        for (string::size_type i = 0; i < rest.length(); i++)
        {
            if ( rest[i] == ':')
                cout++;
            else if (cout == 5)
            {
                if (isdigit(rest[i]))
                    sizes += rest[i];
            }
        }
        //todo does file exists?

        while (atoi(sizes.c_str()) != size)
        {
            bzero(buf, 1024);
            int rec = (int) recv(sock, buf, 1024, 0);
            if (rec < 0)
            {
                errMsg("ERROR: nic neprijato nebo disconnect.");
                remove(localPath.c_str());
                fclose(file);
                return ERR;
            }

            if (fwrite(buf, (size_t )rec, 1, file) != 1)
            {
                errMsg("ERROR: nelze zapsat do souboru.");
                remove(localPath.c_str());
                fclose(file);
                return ERR;
            }
            size += rec;
        }

        if ( atoi(sizes.c_str()) != size)
        {
            errMsg("ERROR: pokazeny soubor");
            remove(localPath.c_str());
            fclose(file);
            return ERR;

        }

        fclose(file);

    }
    else if ( type == LST)
    {
        rest = createHeader(type, path);

        writeSock(sock, rest);


        rest = readSock(sock);
        errorControl = getFirstLineOfResponse(rest);
        if (errorControl == "HTTP/1.1 200 OK\n") {
            rest = readSock(sock);
            cout << rest;
        }
        else {
            rest = readSock(sock);

            cerr << rest << endl;
            return ERR;
        }
    }
    return OK;

}

int del(int sock, string path, int type)
{
    string rest;
    char buf[1024];

    rest = createHeader(type, path);
    // cout << rest << endl;
    bzero(buf, 1024);

    writeSock(sock, rest);

    rest = getFirstLineOfResponse(readSock(sock));

    if (rest != "HTTP/1.1 200 OK\n")
    {
        cerr << readSock(sock);
        return ERR;
    }
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