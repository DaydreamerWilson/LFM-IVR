#include "mlgi_network.h"
#include "mlgi_include.h"

WSADATA wasdata;
ADDRINFO *result = NULL,
        *ptr = NULL,
        hints;
SOCKET ConnectSocket;

std::chrono::system_clock network_clock;

DWORD timeout = 100; // In millisecond

int initiateConnection(char* IP_ADDRESS){
    char* addr = IP_ADDRESS;
    
    int iResult = WSAStartup(MAKEWORD(2, 2), &wasdata);
    if(iResult != 0){
        return WSASTARTUP_FAILED;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    //printf("Connecting to device at <%s>\n", IP_ADDRESS);

    // Resolve server address
    iResult = getaddrinfo(addr, DEFAULT_PORT, &hints, &result);
    if(iResult != 0){
        WSACleanup();
        return GETADDRINFO_FAILED;
    }
    
    // Creating socket to connect to server
    ConnectSocket = INVALID_SOCKET;
    ptr = result;

    while(ConnectSocket == INVALID_SOCKET){
        ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

        if(ConnectSocket == INVALID_SOCKET){
            freeaddrinfo(result);
            WSACleanup();
            return SOCKET_INVALID;
        }

        iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if(iResult == SOCKET_ERROR){
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            return ERROR_SOCKET;
        }
        if(ptr->ai_next != NULL){
            ptr = ptr->ai_next;
        }
        else{
            break;
        }
    }

    if(ConnectSocket == INVALID_SOCKET){
        freeaddrinfo(result);
        WSACleanup();
        return CONNECTION_FAILED;
    }
    else{
        u_long iMode = 1;
        int yes_ = 1;
        setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        iResult = ioctlsocket(ConnectSocket, FIONBIO, &iMode);
        setsockopt(ConnectSocket, IPPROTO_TCP, TCP_NODELAY, (char *)&yes_, sizeof(int));
        setsockopt(ConnectSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&yes_, sizeof(int));
        return CONNECTION_CLEAN;
    }
}

int pushSend(char* SendBuf){
    // Send an initial buffer
    int iResult = send(ConnectSocket, SendBuf, (int) strlen(SendBuf), 0);
    if (iResult == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return SEND_FAILED;
    }
    return iResult;
}

int getRecv(char* &RecvBuf){
    int iResult = recv(ConnectSocket, RecvBuf, DEFAULT_BUFLEN, 0);
        if (iResult > 0){
            return iResult;
        }
        return iResult;
}

static int byteRecv = 0;
static int packetRecv = 0;

using namespace std::chrono_literals;
std::chrono::time_point<std::chrono::system_clock> pushStart = network_clock.now();
std::chrono::time_point<std::chrono::system_clock> recvStart = network_clock.now();

#define cacheImgCount 256 // Theoretical upper limit exists at 16*16
int currentCacheImg = 0;
char *imgCachePath[cacheImgCount];
int lastFmt = -1 ;

void initImgCachePath(_network *network, _gui *gui){
    if(lastFmt!=gui->output.imgFmt){
        for(int i = 0; i < cacheImgCount; i++){
            imgCachePath[i] = new char[32];
            sprintf(imgCachePath[i], "cache/img/im**%s", imgFormat[gui->output.imgFmt]);
            if(i/16>9){
                imgCachePath[i][12] = 'A'+(i/16-10);
            }
            else{
                imgCachePath[i][12] = '0'+(i/16);
            }
            if(i%16>9){
                imgCachePath[i][13] = 'A'+(i%16-10);
            }
            else{
                imgCachePath[i][13] = '0'+(i%16);
            }
            //printf("%s\n", imgCachePath[i]);
        }
        lastFmt = gui->output.imgFmt;
    }
}

void network_loop(_network *network, _gui *gui){
    initImgCachePath(network, gui);
    int pushCounter = 0;
    int recvCounter = 0;
    int timelapseCache = 0;
    int EOB = true; // End of JPEG identifier
    int RST = true; // Force reset connection
    FILE *cacheIMGFile = NULL;
    char* recvBuf = new char[DEFAULT_BUFLEN];
    while(network->input.serviceUp == true){
        Sleep(100);
        initImgCachePath(network, gui);
        if(network->input.connectCmd == doConnect && network->output.connectStatus == disconnected){
            network->output.connectStatus = connecting;
            network->output.connectError = initiateConnection(network->input.addrBuf);
            if(!network->output.connectError){
                //printf("Network up: %d\n",network->output.connectError);
                network->output.connectStatus = connected;
            }
            else{
                printf("Network error: %d\n",network->output.connectError);
                network->output.connectStatus = disconnected;
            }
        }
        if(network->input.connectCmd == doDisconnect && network->output.connectStatus == connected){
            shutdown(ConnectSocket, 2);
            network->output.connectStatus = disconnected;
            getRecv(recvBuf);
            RST = true;
            pushCounter = 0;
            recvCounter = 0;
        }
        if(network->output.connectStatus == connected){
            //std::cout << "Getting message" << std::endl;
            int bufLen = getRecv(recvBuf);
            //std::cout << bufLen << std::endl;
            if(bufLen > 1){
                if(gui->output.stillProgress){
                    gui->output.stillProgress = 2;
                }
                if(byteRecv == 0){
                    recvStart = std::chrono::system_clock::now();
                }
                RST = false;
                EOB = false;
                if(cacheIMGFile!=NULL){
                    fwrite(recvBuf, sizeof(char), bufLen, cacheIMGFile);
                }
                byteRecv += bufLen;
                packetRecv++;
                //printf("%d %d\n", bufLen, byteRecv);
            }
            if(bufLen > 1 && recvBuf[bufLen-1]==0xffffffd9 && recvBuf[bufLen-2]==0xffffffff){
                EOB = true;
                pushCounter = 0;
                recvCounter = 0;
                //printf("EOB detected\n");
            }
            if(EOB == true || RST == true){
                if(recvCounter==0){
                    printf("Total of %d bytes received in %d packets\n", byteRecv, packetRecv);
                    byteRecv = 0;
                    packetRecv = 0;
                    network->output.lastIMGtick = std::chrono::duration_cast<std::chrono::milliseconds>(network_clock.now()-pushStart).count();
                    network->output.IMGtick = 0.8*(float)network->output.IMGtick + 0.2*(float)network->output.lastIMGtick;
                    printf("Transmission took: %d ms / %d ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(network_clock.now()-pushStart).count(), std::chrono::duration_cast<std::chrono::milliseconds>(network_clock.now()-recvStart).count());
                    recvCounter++;
                }
                if(cacheIMGFile!=NULL && EOB && !RST && pushCounter==0){
                    printf("Saving image\n");
                    fclose(cacheIMGFile);
                    network->output.IMGcounter = currentCacheImg;
                    network->output.cachePath = imgCachePath[currentCacheImg++];
                    if(gui->output.timeLapseOn && timelapseCache){
                        timelapseCache = 0;
                        copyFile(network->output.cachePath, gui->input.photoTime);
                    }
                    currentCacheImg %= cacheImgCount;
                    if(gui->output.stillProgress==2){
                        gui->output.stillProgress = 0;
                    }
                    RST = true;
                }
                if(pushCounter==0){
                    if(gui->output.stillProgress==1 || gui->output.getStream){
                        if(!gui->output.timeLapseOn || std::chrono::duration_cast<std::chrono::milliseconds>(network_clock.now()-pushStart).count() > timelapse_interval_t[gui->output.timeLapse_t]){
                            printf("Pushing message\n");
                            char pushBuf[25];
                            memcpy(pushBuf, "/", sizeof(char)*1);
                            memcpy(pushBuf+1, network->input.sendBuf, sizeof(char)*24);
                            pushSend((char*)pushBuf);
                            cacheIMGFile = fopen(imgCachePath[currentCacheImg], "wb");
                            if(gui->output.timeLapseOn){
                                std::time_t t = std::time(0);
                                std::tm* now = std::localtime(&t);
                                sprintf(gui->input.photoTime, "images/timelapse/%d-%d-%d-%d-%d-%d%s", now->tm_year+1900, now->tm_mon+1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec, imgFormat[gui->output.imgFmt]);
                                timelapseCache = 1;
                            }
                            pushStart = std::chrono::system_clock::now();
                            pushCounter++;
                        }
                    }
                    else{
                        pushSend("0");
                    }
                }
                else{
                    if(std::chrono::duration_cast<std::chrono::milliseconds>(network_clock.now()-pushStart).count() > 2000){
                        //printf("Retrying pushing message\n");
                        pushCounter = 0;
                    }
                }
            }
        }
    }
    if(cacheIMGFile!=NULL){
        fclose(cacheIMGFile);
    }
}