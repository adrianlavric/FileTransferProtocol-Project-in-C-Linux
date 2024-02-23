#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <netdb.h>
#include <arpa/inet.h>


/* portul folosit */
#define PORT 2024

/* codul de eroare returnat de anumite apeluri */
extern int errno;

int sterge_director(char cale[]) {

    DIR *director;
    struct dirent *director_curent;
    director = opendir(cale);
    struct stat info_folder;

    if(director) {

        while((director_curent = readdir(director)) != NULL) {

            if(strcmp(director_curent->d_name, ".") != 0 && strcmp(director_curent->d_name, "..")!= 0) {

                char cale_director_curent[500];
                strcpy(cale_director_curent, cale);
                strcat(cale_director_curent, "/");
                strcat(cale_director_curent, director_curent->d_name);
                if(stat(cale_director_curent, &info_folder) == 0) {

                    if(S_ISDIR(info_folder.st_mode)) {

                        int stergere_subdirector = sterge_director(cale_director_curent);                        
                        if(!stergere_subdirector) {

                            return 0;

                        }

                    } else {

                        if(remove(cale_director_curent) != 0) {

                            return 0;

                        }

                    }

                } else {

                    return 0;

                }
            
            }

        }
        closedir(director);

    } else {

        return 0;

    }

    if(rmdir(cale) == 0) {

        return 1;

    } else {

        return 0;

    }

} 

int main() {

    struct sockaddr_in server;	// structura folosita de server
    struct sockaddr_in from;
    char comanda[100];		// comanda primita de la client
    int lungime_comanda;
    int sd;			// descriptorul de socket
    int nr_client = 0;

    // crearea unui socket 
    if((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {

        perror("[server]Eroare la creare socket.\n");
    	return errno;

    }

    // pregatirea structurilor de date 
    bzero(&server, sizeof (server));
    bzero(&from, sizeof (from));

    // umplem structura folosita de server 
    // stabilirea familiei de socket-uri 
    server.sin_family = AF_INET;
    // acceptam orice adresa 
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    // utilizam un port utilizator 
    server.sin_port = htons(PORT);

    // reutilizare adresa
    int reuse = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {

        perror("[server]Eroare la reutilizare adresa.\n");
        return errno;

    }

    // atasam socketul
    if (bind(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
    	perror("[server]Eroare la bind.\n");
    	return errno;
    }

    // punem serverul sa asculte daca vin clienti sa se conecteze 
    if (listen(sd, 10) == -1)
    {
    	perror("[server]Eroare la listen.\n");
    	return errno;
    }

    printf("[server]Asteptam la portul %d...\n", PORT);
    fflush(stdout);

    // servim in mod concurent clientii...
    while(1)
    {
        int client;
    	int length = sizeof(from);

        // acceptam un client (stare blocanta pana la realizarea conexiunii) 
    	client = accept(sd, (struct sockaddr *)&from, &length);
        
        // eroare la acceptarea conexiunii de la un client 
    	if(client < 0)
    	{
    		perror("[server]Eroare la accept.\n");
    		continue;
    	}

        ++nr_client;
        
        int pid;
        if((pid = fork()) == -1) {

            perror("[server]Eroare la fork.\n");
            return errno;

        } else if(pid > 0) {

            // parinte
    		close(client);
    		while(waitpid(-1,NULL,WNOHANG));
    		continue;

        } else if(pid == 0) {

            //copil
            close(sd);
            int logat = 0;
            char user[20];
            char cale_server[500];
            strcpy(cale_server, "/home/adrian/proiect_retele/server");
            int lungime_cale_server = strlen(cale_server);
            cale_server[lungime_cale_server] = '\0';

            char raspuns[200]; int lungime_raspuns;

            printf("[server]Clientul %d s-a conectat la server.\n", nr_client);

            while(1) {

                bzero(comanda, 100);

                // citirea comenzii
    		    if(read(client, comanda, 100) <= 0) {

    			    printf("[server]Eroare la read de la client.\n");
                    printf("[server]Clientul %d a parasit serverul.\n", nr_client);
    		        close(client);	// inchidem conexiunea cu clientul 
    		        exit(0);	
    	        
                }

                lungime_comanda = strlen(comanda);

                // pregatim mesajul de raspuns 
                bzero(raspuns, 200);

                if(strncmp(comanda, "login ", 6) == 0 && logat == 1) {

                    strcpy(raspuns, "Esti deja logat, deconecteaza-te daca vrei sa schimbi userul.");

                } else if(strncmp(comanda, "login ", 6) == 0 && logat == 0) {

                    // decriptez parola, verific daca parametrii sun ok apoi caut in whitelist

                    int nr_spatii = 0;

                    for(int i = 6; i < lungime_comanda; i++) {

                        if(comanda[i] == ' ') {

                            nr_spatii++;

                        }

                        if(nr_spatii == 1 && comanda[i] == ' ') {

                            for(int j = i + 1; j < lungime_comanda; j++) {

                                comanda[j] = comanda[j] - 3;

                            }

                        }

                    }

                    if(nr_spatii > 1 || nr_spatii == 0) {

                        strcpy(raspuns, "Parametri invalizi, sintaxa : login <user> <parola>");

                    } else {

                        char *user_parola = comanda + 6;
                        int ok = 0;

                        FILE* whitelist = fopen("whitelist.txt", "r");
                        if(whitelist) {

                            char info[100];
                            while(fgets(info, sizeof(info), whitelist)) {

                                int lungime_info = strlen(info);
                                info[lungime_info - 1] = '\0';

                                if(strcmp(user_parola, info) == 0) {

                                    ok = 1;     
                                    char *user_curent = strtok(user_parola, " ");
                                    strcpy(user, user_curent);
                                    logat = 1;
                                    printf("[server]Userul %s s-a logat.\n", user);
                                    break;                        

                                }

                            }
                            fclose(whitelist);

                        }

                        if(ok) {

                            strcpy(raspuns, "Te-ai logat cu succes.");

                        } else {

                            strcpy(raspuns, "User sau parola invalida, incearca din nou.");

                        }

                    }

                } else if(strcmp(comanda, "logout") == 0 && logat == 0) {

                    strcpy(raspuns, "Nu esti logat deci nu te poti deconecta, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strcmp(comanda, "logout") == 0 && logat == 1) {

                    printf("[server]Userul %s s-a deconectat.\n", user);  
                    bzero(user, 20);
                    logat = 0;
                    strcpy(raspuns, "Te-ai deconectat cu succes.");  
                    bzero(cale_server, 500); 
                    strcpy(cale_server, "/home/adrian/proiect_retele/server");
                    lungime_cale_server = strlen(cale_server);
                    cale_server[lungime_cale_server] = '\0';

                } else if(strcmp(comanda, "help") == 0 && logat == 0) {

                    strcpy(raspuns, "Comenzi disponibile :\n");
                    strcat(raspuns, "login <user> <parola> | quit\n");
                    strcat(raspuns, "Logheaza-te pentru a avea acces la toate comenzile");

                } else if(strcmp(comanda, "help") == 0 && logat == 1) {

                    strcpy(raspuns, "Comenzi disponibile :\n");
                    strcat(raspuns, "login <user> <parola> | logout | quit\n");
                    strcat(raspuns, "list | listc | pwds | pwdc | mkd <nume> | rmd <nume>\n");
                    strcat(raspuns, "cwds <.. sau nume> | cwdc <.. sau nume sau cale>\n");
                    strcat(raspuns, "upload <nume> | download <nume> | dlf <nume> | rnm <nume> <nume nou>");

                } else if(strcmp(comanda, "quit") == 0) {

                    strcpy(raspuns, "Te-ai deconectat de la server");

                } else if(strcmp(comanda, "pwds") == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strcmp(comanda, "pwds") == 0 && logat == 1) {

                    strcpy(raspuns, cale_server);

                } else if(strncmp(comanda, "cwds ", 5) == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strncmp(comanda, "cwds ", 5) == 0 && logat == 1) {

                    if(strlen(comanda) == 5) {

                        strcpy(raspuns, "Lipseste argumentul, sintaxa : cwdc <.. sau nume_folder>");

                    } else {

                        char *parametru = comanda + 5;
                        if(strcmp(parametru, "..") == 0) {

                            if(strcmp(cale_server, "/home/adrian/proiect_retele/server") == 0) {

                                strcpy(raspuns, "Esti deja in directorul radacina.");

                            } else {

                                char *director_eliminat = strrchr(cale_server, '/');
                                if(director_eliminat) {

                                    *director_eliminat = '\0'; 

                                }

                                strcpy(raspuns, "Locatia a fost schimbata la ");
                                strcat(raspuns, cale_server);
                                strcat(raspuns, ".");

                            }

                        } else {

                            char cale_temp[500];
                            strcpy(cale_temp, cale_server);
                            strcat(cale_temp, "/");
                            strcat(cale_temp, parametru);
                            int lungime_cale_temp = strlen(cale_temp);
                            cale_temp[lungime_cale_temp] = '\0';
                        
                            struct stat info_cale;
                            if(stat(cale_temp, &info_cale) == 0) {

                                if(S_ISDIR(info_cale.st_mode)) {

                                    DIR *director;
                                    director = opendir(cale_temp);
                                    if(director) {

                                        bzero(cale_server, 500);
                                        strcpy(cale_server, cale_temp);
                                        lungime_cale_server = strlen(cale_server);
                                        cale_server[lungime_cale_server] = '\0';
                                        strcpy(raspuns, "Locatia a fost schimbata la ");
                                        strcat(raspuns, cale_server);
                                        strcat(raspuns, ".");
                                        closedir(director);

                                    } else {

                                        strcpy(raspuns, "Eroare la deschidere director.");

                                    }   

                                } else {

                                    strcpy(raspuns, "Nu exista niciun director cu acest nume.");

                                }

                            } else {

                                strcpy(raspuns, "Argument invalid, sintaxa : cwds <.. sau nume_folder>");                  

                            }

                        }

                    }

                    if(strlen(raspuns) == 0) {

                        strcpy(raspuns, "Eroare la schimbare director.");
                    }

                } else if(strncmp(comanda, "mkd ", 4) == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strncmp(comanda, "mkd ", 4) == 0 && logat == 1) {

                    if(strlen(comanda) == 4) {

                        strcpy(raspuns, "Lipseste argumentul, sintaxa : mkd <nume>");

                    } else {

                        char cale_creare[500];
                        strcpy(cale_creare, cale_server);
                        strcat(cale_creare, "/");
                        char *nume_folder = comanda + 4;
                        strcat(cale_creare, nume_folder);

                        if(mkdir(cale_creare, 0777) == 0) {

                            strcpy(raspuns, "Directorul a fost creat cu succes si are calea urmatoare :");
                            strcat(raspuns, cale_creare);

                        } else {

                            if(errno == EEXIST) {

                                strcpy(raspuns, "Directorul exista deja.");

                            } else {

                                strcpy(raspuns, "Eroare la creare director");

                            }

                        }

                    }

                } else if(strncmp(comanda, "rmd ", 4) == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strncmp(comanda, "rmd ", 4) == 0 && logat == 1) {

                    if(strlen(comanda) == 4) {

                        strcpy(raspuns, "Lipseste argumentul, sintaxa : rmd <nume>");

                    } else {

                        char *director_de_sters = comanda + 4;
                        char cale_folder_stergere[500];
                        DIR *director;
                        struct dirent *director_curent;
                        director = opendir(cale_server);
                        struct stat info_folder;
                        if(director) {

                            while((director_curent = readdir(director)) != NULL) {

                                if(strcmp(director_de_sters, director_curent->d_name) == 0) {

                                    strcpy(cale_folder_stergere, cale_server);
                                    strcat(cale_folder_stergere, "/");
                                    strcat(cale_folder_stergere, director_de_sters);

                                    if(stat(cale_folder_stergere, &info_folder) == 0) {

                                        if(S_ISDIR(info_folder.st_mode) <= 0) {

                                            strcpy(raspuns, "Numele introdus nu apartine unui folder valid de pe server.");

                                        } else {

                                            int sters = 0;
                                            sters = sterge_director(cale_folder_stergere);
                                            if(sters) {

                                                strcpy(raspuns, "Directorul a fost sters cu succes.");

                                            } else {

                                                strcpy(raspuns, "Eroare la stergere director");

                                            }

                                        }

                                    } else {

                                        strcpy(raspuns, "Eroare la stat.");

                                    }

                                }

                            }
                            closedir(director);

                            if(strlen(raspuns) == 0) {

                                strcpy(raspuns, "Directorul nu exista.");
                                
                            }

                        } else {

                            strcpy(raspuns, "Eroare la deschidere director curent server.");

                        }

                    }

                } else if(strcmp(comanda, "list") == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strcmp(comanda, "list") == 0 && logat == 1) {

                    DIR *director;
                    struct dirent *director_curent;
                    director = opendir(cale_server);

                    if(director) {

                        strcpy(raspuns, "Continutul directorului curent pe server :");
                        while((director_curent = readdir(director)) != NULL) {

                            if(strcmp(director_curent->d_name, ".") !=0 && strcmp(director_curent->d_name, "..") != 0) {
                                
                                strcat(raspuns, "\n");
                                strcat(raspuns, director_curent->d_name);
                                
                            }

                        }
                        int lungime_raspuns_newline = strlen(raspuns);
                        raspuns[lungime_raspuns_newline] = '\0';

                        closedir(director);

                    } else {

                        strcpy(raspuns, "Eroare la afisare continut.");

                    }

                    if(strcmp(raspuns, "Continutul directorului curent pe server :") == 0) {

                        strcpy(raspuns, "Nu exista niciun director sau fisier in ");
                        strcat(raspuns, cale_server);
                        strcat(raspuns, ".");

                    }

                } else if(strncmp(comanda, "upload ", 7) == 0 && logat == 1) { //execut upload si download doar daca userul e logat, daca nu logat clientul nu trimite
                    
                    char* fisier_upload = comanda + 7;
                    int nu_exista = 1;
                    DIR *director;
                    struct dirent *director_curent;
                    director = opendir(cale_server);
                    char raspuns1[200]; int lungime_raspuns1;
                    bzero(raspuns1, 200);
                    if(director) {

                        while((director_curent = readdir(director)) != NULL) {

                            if(strcmp(fisier_upload, director_curent->d_name) == 0) {

                                nu_exista = 0;
                                strcpy(raspuns1, "Serverul contine un fisier cu acest nume, il poti redenumi folosind rmd.");

                            }

                        }
                        closedir(director);

                    } else {

                        strcpy(raspuns1, "Erore la deschidere director curent pe server.");

                    }

                    lungime_raspuns1 = strlen(raspuns1);
                    if(lungime_raspuns1) {

                        write(client, &lungime_raspuns1, sizeof(int));
                        write(client, raspuns1, lungime_raspuns1);
                        continue;

                    } else {

                        strcpy(raspuns1, "Ok.");
                        lungime_raspuns1 = strlen(raspuns1);
                        write(client, &lungime_raspuns1, sizeof(int));
                        write(client, raspuns1, lungime_raspuns1);

                    }

                    if(nu_exista) {

                        char cale_fisier_upload[500];
                        strcpy(cale_fisier_upload, cale_server);
                        strcat(cale_fisier_upload, "/");
                        strcat(cale_fisier_upload,  fisier_upload);
                        FILE* fisier_nou = fopen(cale_fisier_upload, "w");
                        if(fisier_nou) {

                            long dimensiune_fisier;
                            read(client, &dimensiune_fisier, sizeof(long));
                            char* continut = (char*)malloc(dimensiune_fisier + 1);
                            read(client, continut, dimensiune_fisier);
                            continut[dimensiune_fisier] = '\0';
                            fwrite(continut, 1, dimensiune_fisier, fisier_nou);
                            fclose(fisier_nou);
                            free(continut);
                            int transfer_succes = 1;
                            write(client, &transfer_succes, sizeof(int));
                            continue;

                        } else {

                            int transfer_succes = 0;
                            write(client, &transfer_succes, sizeof(int));
                            continue;

                        }

                    }
                    continue;

                } else if(strncmp(comanda, "download ", 9) == 0 && logat == 1) {

                    char *fisier_download = comanda + 9;
                    char cale_fisier_download[500];
                    DIR *director;
                    struct dirent *director_curent;
                    director = opendir(cale_server);
                    struct stat info_download;
                    char raspuns1[200]; int lungime_raspuns1;
                    bzero(raspuns1, 200);
                    int fisier_valid = 0;
                    long dimensiune_fisier2;
                    if(director) {

                        while((director_curent = readdir(director)) != NULL) {

                            if(strcmp(fisier_download, director_curent->d_name) == 0) {

                                strcpy(cale_fisier_download, cale_server);
                                strcat(cale_fisier_download, "/");
                                strcat(cale_fisier_download, fisier_download);

                                if(stat(cale_fisier_download, &info_download) == 0) {

                                    if(S_ISREG(info_download.st_mode) <= 0) {

                                        strcpy(raspuns1, "Numele introdus nu apartine unui fisier valid de pe server.");

                                    } else {

                                        FILE* fisier_download = fopen(cale_fisier_download, "r");
                                        if(fisier_download) {

                                            strcpy(raspuns1, "Ok");
                                            fisier_valid = 1;
                                            fclose(fisier_download);

                                        } else {

                                            strcpy(raspuns1, "Eroare la deschidere fisier.");

                                        }

                                    }

                                } else {

                                    strcpy(raspuns1, "Eroare la stat.");

                                }

                            }

                        }
                        closedir(director);

                    } else {

                        strcpy(raspuns1, "Eroare la deschidere director curent server.");

                    }

                    lungime_raspuns1 = strlen(raspuns1);
                    if(lungime_raspuns1 == 0) {
                    
                    	strcpy(raspuns1, "Fisierul nu exista pe server.");
                        lungime_raspuns1 = strlen(raspuns1);
                    
                    }

                    write(client, &lungime_raspuns1, sizeof(int));
                    write(client, raspuns1, lungime_raspuns1);

                    if(!fisier_valid) {

                        continue;

                    }

                    int nu_exista_local = 0;
                    read(client, &nu_exista_local, sizeof(int));
                    
                    if(nu_exista_local) {

                        FILE* fisier_download2 = fopen(cale_fisier_download, "r");
                        fseek(fisier_download2, 0, SEEK_END);
                        dimensiune_fisier2 = ftell(fisier_download2);
                        fseek(fisier_download2, 0, SEEK_SET);
                        char* continut2 = (char*)malloc(dimensiune_fisier2 + 1);
                        fread(continut2, 1, dimensiune_fisier2, fisier_download2);
                        continut2[dimensiune_fisier2] = '\0';
                        fclose(fisier_download2); 
                        write(client, &dimensiune_fisier2, sizeof(long));
                        write(client, continut2, dimensiune_fisier2);
                        free(continut2);
                        continue;

                    }

                    continue;

                } else if(strncmp(comanda, "dlf ", 4) == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strncmp(comanda, "dlf ", 4) == 0 && logat == 1) {

                    if(strlen(comanda) == 4) {

                        strcpy(raspuns, "Lipseste argumentul, sintaxa : dlf <nume>");

                    } else {

                        char* fisier_de_sters = comanda + 4;
                        DIR *director;
                        struct dirent *director_curent;
                        director = opendir(cale_server);
                        struct stat info_delete;
                        char cale_delete[500];

                        if(director) {

                            while((director_curent = readdir(director)) != NULL) {
                    
                                if(strcmp(fisier_de_sters,director_curent->d_name) == 0) {

                                    strcpy(cale_delete, cale_server);
                                    strcat(cale_delete, "/");
                                    strcat(cale_delete, fisier_de_sters);

                                    if(stat(cale_delete, &info_delete) == 0) {

                                        if(S_ISREG(info_delete.st_mode) <= 0) {

                                            strcpy(raspuns, "Numele nu apartine unui fisier valid.");

                                        } else {

                                            if(remove(cale_delete) == 0) {

                                                strcpy(raspuns, "Fisierul a fost sters cu succes.");

                                            } else {

                                                strcpy(raspuns,  "Eroare la stergere.");

                                            }

                                        }

                                    } else {

                                        strcpy(raspuns, "Eroare la stat.");

                                    }

                                }

                                if(strlen(raspuns) == 0) {

                                    strcpy(raspuns, "Fisierul nu exista.");

                                }

                            }
                            closedir(director);

                        } else {

                            strcpy(raspuns, "Eroare la deschidere director curent server.");
                        
                        }

                    }

                } else if(strncmp(comanda, "rnm ", 4) == 0 && logat == 0) {

                    strcpy(raspuns, "Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.");

                } else if(strncmp(comanda, "rnm ", 4) == 0 && logat == 1) {

                    if(strlen(comanda) == 4) {

                        strcpy(raspuns, "Lipsesc argumentele, sintaxa : rnm <nume> <nume nou>");

                    } else {

                        char* nume = comanda + 4;
                        int i = 0;
                        int nr_spatii = 0;
                        while(nume[i] != '\0') {

                            if(nume[i] == ' ') {

                                nr_spatii++;

                            }
                            i++;

                        }

                        if(nr_spatii != 1) {

                            strcpy(raspuns, "Parametri invalizi, sintaxa : rnm <nume> <nume nou>");

                        } else {

                            char* nume_vechi = strtok(nume, " ");
                            char* nume_nou = strtok(NULL, " ");

                            DIR *director;
                            struct dirent *director_curent;
                            director = opendir(cale_server);
                            struct stat info_rename;
                            char cale_veche[500];
                            char cale_noua[500];

                            if(director) {

                                while((director_curent = readdir(director)) != NULL) {
                    
                                    if(strcmp(nume_vechi, director_curent->d_name) == 0) {

                                        strcpy(cale_veche, cale_server);
                                        strcat(cale_veche, "/");
                                        strcat(cale_veche, nume_vechi);

                                        if(stat(cale_veche, &info_rename) == 0) {   

                                            if(S_ISREG(info_rename.st_mode) <= 0) {

                                                strcpy(raspuns, "Numele nu apartine unui fisier valid.");

                                            } else {

                                                strcpy(cale_noua, cale_server);
                                                strcat(cale_noua, "/");
                                                strcat(cale_noua, nume_nou);

                                                if(rename(cale_veche, cale_noua) == 0) {

                                                    strcpy(raspuns, "Fisierul a fost redenumit cu succes.");

                                                } else {

                                                    strcpy(raspuns, "Eroare la redenumire.");

                                                }

                                            }

                                        } else {

                                            strcpy(raspuns, "Eroare la stat.");

                                        }

                                    }

                                    if(strlen(raspuns) == 0) {

                                        strcpy(raspuns, "Fisierul nu exista.");

                                    }

                                }
                                closedir(director);
                            
                            } else {

                                strcpy(raspuns, "Eroare la deschidere director curent server.");
                        
                            }
                            
                        }
                    
                    }

                }
 
                lungime_raspuns = strlen(raspuns);

                // daca raspunsul e null inseamna ca nu s-a intrat in niciun if

                if(lungime_raspuns == 0) {

                    strcpy(raspuns, "Comanda invalida, introdu <help> pentru a vedea comenzile disponibile");
                    lungime_raspuns = strlen(raspuns);

                }

                // trimit raspunsul la client

                if(write(client, &lungime_raspuns, sizeof(int)) <= 0) {

                    perror("[server]Eroare la write catre client.\n");
    		        exit(0);

                }

    		    if(write(client, raspuns, lungime_raspuns) <= 0)
    		    {

    		        perror("[server]Eroare la write catre client.\n");
    		        exit(0);

    	        }

                if(strcmp(raspuns, "Te-ai deconectat de la server") == 0) {

                    printf("[server]Clientul %d a parasit serverul.\n", nr_client);
                    close(client);
                    exit(0);

                }
                
            }

        }

    }

}