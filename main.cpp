#include <iostream>
#include <vector>
#include <string>
#include <locale>
#include <regex>
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

#define PUT 1
#define DEL 2
#define GET 3
#define LST 4
#define MKD 5
#define RMD 6

#define ERR 1
// localhost    (?<=\/)([a-zA-Z0-9\.]*)(?=:)
// port         (?<=:)[0-9]*
// path         (?<=[0-9])\/.*
using namespace std;
vector<string> commands = { "PUT", "DEL", "GET", "LST", "MKD", "RMD"};
struct arg {
    string portNum;
    string type;
    string ipAddr;
    string localPath;
    string remotePath;
    int alpha, j;
};
//regex localhost ("(?<=\\/)([a-zA-Z0-9\\.]*)(?=:)");
//regex port  ("(?<=:)[0-9]*");
//regex path ("(?<=[0-9])\\/.*");
void errMsg(int err,const char *msg)
{
    perror(msg);
    exit(err);
}

arg* getParams(int argc, char *argv[])
{
    char temp[PATH_MAX];
    arg * str = new arg;
    if (argc != 4 && argc != 3)
        errMsg(42, "ERROR: spatne argumetny");


    locale l;
    string arg(argv[1]);

    str->alpha = 42;
    str->j = 0;

    for (string::size_type i = 0; i < arg.length(); i++ )
    {
        arg[i] = toupper(arg[i], l);
    }


    while (str->j < 6 && str->alpha != 0) {
        //cout << arg << endl ;
        str->alpha = arg.compare(commands[str->j]);
        str->j++;
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
            //cout << tmp << end
            // l;
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
        //cout << tmp << endl;
    }
    //cout << str->ipAddr <<endl;
    //cout << str->portNum << endl;
    //cout << str->remotePath << endl;
    if (str->portNum.empty())
        str->portNum = "6677";
    if (str->ipAddr.empty() || str->portNum.empty() || str->remotePath.empty() )
        errMsg(ERR, "Spatne zadane parametry");

    return str;
}

void put (int sock, string path, int type, string localPath)
{
    FILE *file, *mime;
    int n;
    long fsize;
    struct stat s;
    char buf[1024];
    char *content = NULL;
    string rest;
    string typeOfMsg;
    time_t now;
    struct tm tm;

    if( stat(localPath.c_str(),&s) == 0 )
    {
        if( s.st_mode & S_IFDIR )
        {
            typeOfMsg = "?type=folder HTTP/1.1";
            // cout << localPath + "dir" << endl;
        }
        else if( s.st_mode & S_IFREG )
        {
            file = fopen(localPath.c_str(), "rb");
            typeOfMsg = "?type=file HTTP/1.1";
            if ( file == NULL ) errMsg(42, "ERROR otevirani souboru");
            //cout << localPath + "file" << endl;
        }
        else
        {
            errMsg(42, "ERROR ani slozka, ani soubor. ");
        }
    }
    else
    {
        errMsg(42, "ERROR nenalezena cesta");
    }
    if (type == PUT)
        rest = "PUT ";
    else if (type == MKD)
        rest = "MKD ";

    rest.append(path);
    rest.append(typeOfMsg);

    now = time(0);
    tm = *gmtime(&now);
    strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", &tm);

    rest += "\r\nDate: ";
    rest.append(buf);
    //todo content type in case of folder - txt?
    rest += "\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nContent-Type: application/octet-stream\r\nContent-Length: ";

    if (type == PUT) {
        file = fopen(localPath.c_str(), "rb");

        if (!file) errMsg(42, "ERROR pri otevirani souboru");

        if (fseek(file, 0, SEEK_END) == -1) errMsg(42, "ERROR nelze dojit na konec souboru");

        fsize = ftell(file);
        //  cout << fsize << endl;
        if (fsize == -1) errMsg(42, "ERROR zadna velikost souboru");

        rest += to_string(fsize) + "\n\r";

        rewind(file);
        string a = "file -b --mime-type " + localPath;
        if ((mime = popen(a.c_str(), "r")) == NULL)
            errMsg(42, "ERROR popen neziskal mime");

        char b[200];
        string mi;
        while(fgets(b, 200, mime))
            mi += b;
        cout << mi << endl;
        pclose(mime);
        content = (char *) malloc((size_t) fsize);
        if (!content) errMsg(42, "ERROR nenaalokoval se prostor pro file");

        if (fread(content, fsize, 1, file) != 1) errMsg(42, "ERROR soubor nebyl nacten");
        fclose(file);

        //cout<< fsize <<endl;
        n = (int) write(sock, rest.data(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");

        bzero(buf, 1024);
        n = (int) read(sock, buf, 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");
        if (strcmp(buf, "HTTP/1.1 200 OK") != 0) errMsg(42, "ERROR neco se pokazilo u respondu.");

        while (fsize > 0) {
            int sent = (int) send(sock, content, fsize, 0);
            if (sent <= 0) {
                if (sent == 0) errMsg(42, "ERROR disconnected");
                else errMsg(42, "ERROR nic nebylo zapsano");
            }
            content += sent;
            fsize -= sent;
        }
        bzero(buf, 1024);
        n = (int) read(sock, buf, 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");
        if (strcmp(buf, "HTTP/1.1 200 OK") != 0) errMsg(42, "ERROR neco se pokazilo u respondu.");

        cout << buf << endl;
    }
    else if (type == MKD)
    {
        bzero(buf, 1024);
        rest += "0 \n\r";
        n = (int) write(sock, rest.data(), 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");

        n = (int) read(sock, buf, 1024);
        if (n < 0) errMsg(42, "ERROR cteni ze socketu");
        if (strcmp(buf, "HTTP/1.1 200 OK") != 0) errMsg(42, "ERROR neco se pokazilo u respondu.");
        cout << buf << endl;

    }
    //free(content);
    close(sock);

}

void get(int sock, string path, int type, string localPath)
{

}

void del(int sock, string path, int type)
{

}



int main(int argc , char *argv[])
{

    arg *str = getParams(argc, argv);


    int sockfd, portno, n;
    sockaddr_in serv_addr;
    hostent *server = NULL;

    portno = stoi(str->portNum);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
        errMsg(ERR, "Nevytvořil se socket");

    server = gethostbyname(str->ipAddr.c_str());

    if (server == NULL)
        errMsg(ERR, "Nevytvořil hosta");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family    =   AF_INET;
    bcopy((char *) server->h_addr, (char*)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port  =   htons(portno);

    if (connect(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        errMsg(ERR, "ERROR connectiong");

    if (str->alpha == 0)
    {   // cout << commands[PUT-1].c_str() << endl;
        switch (str->j)
        {
            case PUT:

                put(sockfd, str->remotePath, PUT, str->localPath);

                break;
            case DEL:

                del(sockfd, str->remotePath, DEL);
                break;
            case GET:
                get(sockfd, str->remotePath, GET , str->localPath);

                break;
            case LST:
                get(sockfd, str->remotePath, LST , NULL);
                break;
            case MKD:
                put(sockfd, str->remotePath, MKD, str->localPath);

                break;
            case RMD:
                del(sockfd, str->remotePath, RMD);
                break;
            default:
                cerr << "Hello Chuck! I've been waiting for you" << endl;
        }
    }
    else
    {
        delete str;
        cerr << "Neznámý parametr" << endl;
        exit(1);
    }
    delete str;
    return 0;
}