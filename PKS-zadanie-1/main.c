#include "header.h"

int main() {
    WSADATA wsa;
    char *ip_adress = (char *) malloc((MAX_IP_LENGTH + 1) * sizeof(char));
    int port_number;
    SOCKET my_socket;
    struct sockaddr_in socket_config;
    CONFIGURATION *socket_with_config;
    char ser_client;

    set_global_variables();

    printf("\nInicializacia Winsocku...");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        getchar();
        exit(EXIT_FAILURE);
    }
    printf("Inicializovane.\n");

    //po inicializacii potrebujem IPčku kam posielam/prijimam
    printf("IP adresu druheho pocitaca:\n");
    scanf("%s", ip_adress);

    //teraz potrebujem port
    printf("Port:\n");
    scanf("%d", &port_number);

    //velkost fragmentu nesmie byt vacsia ako 1500 dokopy (data + hlavicka)
    //ak je mensia ako velkost hlavicky + aspon jeden B dat, nema zmysel nieco vobec posielat
    printf("Velkost fragmentu:\n");
    scanf("%d", &max_size_of_packet);
    if(max_size_of_packet < 16 + 1){
        printf("Velkost fragmentu nie je dostacujuca pre hlaciku.\n");
        exit(-1);
    }
    if(max_size_of_packet > 1456){
        printf("Velkost fragmentu je prilis velka, dochadza k fragmentacii na linkovej vrstve\n");
        exit(-1);
    }

    //vytvorime socket
    if ((my_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Nie je mozne vytvorit socket, kod chyby: %d", WSAGetLastError());
    }

    //konfigurujem socket
    socket_config.sin_family = AF_INET;
    socket_config.sin_addr.s_addr = inet_addr(ip_adress);
    socket_config.sin_port = htons(port_number);

    //vytvorim a nastavim wrapper (a dáme si wrapík :) Som hladna)
    socket_with_config = (CONFIGURATION *) malloc(sizeof(CONFIGURATION));
    socket_with_config->socket = my_socket;
    socket_with_config->socket_config = socket_config;

    //opytam sa ci chcem byt klient alebo server
    printf("Chcem byt server(s) alebo client(c)?\n");
    getchar();
    scanf("%c", &ser_client);
    if (ser_client == 'c') {
        client(socket_with_config);
    } else if (ser_client == 's') {
        server(socket_with_config);
    }

    exit(0);
}

void server(CONFIGURATION *config_socket) {
    struct sockaddr_in server_config;
    char *ip_adress = (char *) malloc((MAX_IP_LENGTH + 1) * sizeof(char));

    //nakonfigurujem "server", tam kde hostujem socket (server musi vediet, ze on hostuje socket)
    printf("IP adresa servera (moja): \n");
    scanf("%s", ip_adress);
    server_config.sin_family = AF_INET;
    server_config.sin_addr.s_addr = inet_addr(ip_adress);
    server_config.sin_port = config_socket->socket_config.sin_port;

    //zavolam bind, adresa moja, nie druheho pocitaca ("ja" hostujem socket)
    if (bind(config_socket->socket, (struct sockaddr *) &server_config, sizeof(server_config)) == SOCKET_ERROR) {
        printf("Funkcia bind prebehla spravne, kod chyby: %d", WSAGetLastError());
        exit(1);
    }
    printf("Bind vykonany.\n");

    //spustim vlakna pre posielanie a prijimanie. Keep-alive server nepotrebuje.
    pthread_t send, receive;
    pthread_create(&send, NULL, sending, config_socket);
    getchar();
    pthread_create(&receive, NULL, receiving, config_socket);
    pthread_join(send, NULL);
    pthread_join(receive, NULL);
}

void client(CONFIGURATION *config_socket) {
    if (connect(config_socket->socket, (struct sockaddr *) &config_socket->socket_config,
                sizeof(config_socket->socket_config)) == SOCKET_ERROR) {
        closesocket(config_socket->socket);
        config_socket->socket = INVALID_SOCKET;
    }

    if (config_socket->socket == INVALID_SOCKET) {
        printf("Nie je mozne pripojit sa k serveru.\n");
        WSACleanup();
        exit(-1);
    }

    //spustim vlakna pre posielanie, prijimanie a keep-alive, ktory posiela client na server
    pthread_t send, receive, keep_alive_t;
    pthread_create(&send, NULL, sending, config_socket);
    pthread_create(&receive, NULL, receiving, config_socket);
    pthread_create(&keep_alive_t, NULL, keep_alive, config_socket);
    pthread_join(send, NULL);
    pthread_join(receive, NULL);
    pthread_join(keep_alive_t, NULL);
}