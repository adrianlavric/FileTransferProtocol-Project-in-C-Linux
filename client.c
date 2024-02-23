#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>

// codul de eroare returnat de anumite apeluri 
extern int errno;

// portul de conectare la server
int port;

int main (int argc, char *argv[]) {

    int sd;			// descriptorul de socket
    struct sockaddr_in server;	// structura folosita pentru conectare 
    char comanda[100];		// comanda trimisa
    int lungime_comanda;
    int logat = 0;
    char cale_client[500];
    if(getcwd(cale_client, sizeof(cale_client)) == NULL) {

      perror("[server]Eroare la citire cale curenta.\n");
      return errno;

    }
    int lungime_cale_client = strlen(cale_client);
    cale_client[lungime_cale_client] = '\0';

    // exista toate argumentele in linia de comanda? 
    if(argc != 3) {

      printf("Sintaxa: %s <adresa_server> <port>\n", argv[0]);
      return -1;
    
    }

    // stabilim portul */
    port = atoi(argv[2]);

    /* cream socketul */
    if((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1) {
      
      perror("Eroare la socket.\n");
      return errno;

    }

    // umplem structura folosita pentru realizarea conexiunii cu serverul 
    // familia socket-ului 
    server.sin_family = AF_INET;
    // adresa IP a serverului 
    server.sin_addr.s_addr = inet_addr(argv[1]);
    // portul de conectare 
    server.sin_port = htons (port);

    // ne conectam la server 
    if (connect(sd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {

      perror("[client]Eroare la connect.\n");
      return errno;

    }

    while(1) {

        // citire comanda 
        bzero(comanda, 100);
        printf("[client]Introduceti o comanda : ");
        fflush(stdout);
        read(0, comanda, 100);
        lungime_comanda = strlen(comanda) - 1;
        comanda[lungime_comanda] = '\0';
        int comanda_locala = 0;

        if(strncmp(comanda, "login ", 6) == 0) {

          //criptez parola direct si verific corectitudinea comenzii in server

          int nr_spatii = 0;

          for(int i = 6; i < lungime_comanda; i++) {

            if(comanda[i] == ' ') {

              nr_spatii++;

            }

            if(nr_spatii == 1) {

              for(int j = i + 1; j < lungime_comanda; j++) {

                comanda[j] = comanda[j] + 3;

              }
              break;

            }

          }

        } else if((strcmp(comanda, "pwdc") == 0 || strncmp(comanda, "cwdc ", 5) == 0) && logat == 0) {

          comanda_locala = 1;
          printf("[client]Raspuns : Trebuie sa fii logat pentru a executa aceste comenzi, introdu <help> pentru a vedea comenzile disponibile.\n");

        } else if((strcmp(comanda, "pwdc") == 0 || strncmp(comanda, "cwdc ", 5) == 0) && logat == 1) {

          comanda_locala = 1;

          if(strcmp(comanda, "pwdc") == 0) {

            printf("[client]Raspuns : %s\n", cale_client);

          } else if(strncmp(comanda, "cwdc ", 5) == 0) {

            if(strlen(comanda) == 5) {

              printf("[client]Raspuns : Lipseste argumentul, sintaxa : cwdc <.. sau cwdc nume_folder sau cale_absoluta.\n");

            } else {

              char *parametru = comanda + 5;
              int cale_absoluta = 0;

              for(int j = 0; j < strlen(parametru); j++) {

                if(parametru[j] == '/') {

                  ++cale_absoluta;

                }

              }

              if(strcmp(parametru, "..") == 0) {

                if(strcmp(cale_client, "/") == 0) {

                  printf("[client]Raspuns : Esti deja in directorul radacina.\n");

                } else {

                  char *director_eliminat = strrchr(cale_client, '/');
                  if(director_eliminat) {

                    *director_eliminat = '\0'; 

                  }

                  if(strlen(cale_client) == 0) {

                    strcpy(cale_client, "/");

                  }
                  printf("[client]Raspuns : Locatia a fost schimbata la %s.\n", cale_client);
                
                }

              } else if(cale_absoluta > 0) {

                struct stat info_cale_absoluta;
                if(stat(parametru, &info_cale_absoluta) == 0) {

                  if(S_ISDIR(info_cale_absoluta.st_mode)) {

                    DIR *director;
                    director = opendir(parametru);
                    if(director) {

                      bzero(cale_client, 500);
                      strcpy(cale_client, parametru);
                      lungime_cale_client = strlen(cale_client);
                      cale_client[lungime_cale_client] = '\0';
                      printf("[client]Raspuns : Locatia a fost schimbata la %s.\n", cale_client);
                      closedir(director);

                    } else {

                      printf("[client]Raspuns : Eroare la deschidere director.\n");

                    }

                  } else {

                    printf("[client]Raspuns : Nu exista niciun director cu acest nume.\n");

                  }

                } else {

                  printf("[client]Raspuns : Cale invalida, sintaxa : cwdc <.. sau nume_folder sau cale_absoluta>.\n");                     

                }

                cale_absoluta = 0;

              } else {

                char cale_temp[500];
                strcpy(cale_temp, cale_client);
                if(strcmp(cale_temp, "/") != 0)
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

                      bzero(cale_client, 500);
                      strcpy(cale_client, cale_temp);
                      lungime_cale_client = strlen(cale_client);
                      cale_client[lungime_cale_client] = '\0';
                      printf("[client]Raspuns : Locatia a fost schimbata la %s.\n", cale_client);
                      closedir(director);

                    } else {

                      printf("[client]Raspuns : Eroare la deschidere director.\n");

                    }

                  } else {

                    printf("[client]Raspuns : Nu exista niciun director cu acest nume.\n");

                  }

                } else {

                  printf("[client]Raspuns : Argument invalid, sintaxa : cwdc <.. sau cwdc nume_folder sau cale_abdoluta..\n");                  

                }

              }

            }

          }

        } else if(strncmp(comanda, "upload ", 7) == 0 && logat == 0) {

          comanda_locala = 1; //ca sa nu mai trimita la server
          printf("[client]Raspuns : Trebuie sa fii logat pentru a executa aceste comenzi, introdu <help> pentru a vedea comenzile disponibile.\n");

        } else if(strncmp(comanda, "upload ", 7) == 0 && logat == 1) {

          if(strlen(comanda) == 7) {

            comanda_locala = 1;
            printf("[client]Raspuns : Lipseste argumentul, sintaxa : upload <nume>.\n");
          
          } else {

            char nume_fisier[20];
            int j = 0;
            for(int i = 7; i < lungime_comanda; i++) {

              nume_fisier[j] = comanda[i];
              j++;

            }
            nume_fisier[j] = '\0';
            char cale_fisier[500];
            DIR *director;
            struct dirent *director_curent;
            director = opendir(cale_client);
            struct stat info_upload;
            int exista = 0;
            long dimensiune_fisier;
            if(director) {

              while((director_curent = readdir(director)) != NULL) {

                if(strcmp(nume_fisier,director_curent->d_name) == 0) {

                  strcpy(cale_fisier, cale_client);
                  strcat(cale_fisier, "/");
                  strcat(cale_fisier, nume_fisier);
              
                  if(stat(cale_fisier, &info_upload) == 0) {

                    if(S_ISREG(info_upload.st_mode) <= 0) {

                      comanda_locala = 1;
                      printf("[client]Raspuns : Numele introdus nu apartine unui fisier valid.\n");
                      continue;

                    } else {

                      exista = 1;

                    }

                  } else {

                    comanda_locala = 1;
                    printf("[client]Raspuns : Eroare la stat.\n");
                    continue;

                  }

                } 

              }
              closedir(director);

            } else {


                comanda_locala = 1;
                printf("[client]Raspuns : Eroare la deschidere director curent.\n");
                continue;

            }

            if(exista) {

              FILE* fisier_upload = fopen(cale_fisier, "r");
              if(fisier_upload) {

                if(write(sd, comanda, 100) <= 0) {

                  perror("[client]Eroare la write spre server.\n");
                  return errno;
        
                }

                char* raspuns1; int lungime_raspuns1;
                read(sd, &lungime_raspuns1, sizeof(int));
                raspuns1 = (char*)malloc(lungime_raspuns1 + 1);
                read(sd, raspuns1, lungime_raspuns1);
                raspuns1[lungime_raspuns1] = '\0';

                if(strcmp(raspuns1, "Serverul contine un fisier cu acest nume, il poti redenumi folosind rmd.") == 0 || strcmp(raspuns1, "Erore la deschidere director curent pe server.") == 0) {

                  printf("[client]Raspuns : %s\n", raspuns1);
                  continue;

                }

                // incep sa trimit             
                
                fseek(fisier_upload, 0, SEEK_END);
                dimensiune_fisier = ftell(fisier_upload);
                fseek(fisier_upload, 0, SEEK_SET);
                char* continut = (char*)malloc(dimensiune_fisier + 1);
                fread(continut, 1, dimensiune_fisier, fisier_upload);
                continut[dimensiune_fisier] = '\0';

                write(sd, &dimensiune_fisier, sizeof(long));
                write(sd, continut, dimensiune_fisier);
                free(continut);
                
                int transfer_succes = 0;
                read(sd, &transfer_succes, sizeof(int));

                if(transfer_succes) {

                  printf("[client]Raspuns : Fisierul a fost incarcat pe server.\n");

                } else {

                  printf("[client]Raspuns : Eroare la transfer.\n");

                }

                fclose(fisier_upload); 
                continue;

              } else {

                comanda_locala = 1;
                printf("[client]Raspuns : Eroare la deschidere fisier.\n");

              }

            } else {

              if(!comanda_locala) {

                comanda_locala = 1;
                printf("[client]Raspuns : Fisierul nu exista.\n");   

              }           

            }

          }

        } else if(strncmp(comanda, "download ", 9) == 0 && logat == 0) {

          comanda_locala = 1;
          printf("[client]Raspuns : Trebuie sa fii logat pentru a executa aceste comenzi, introdu <help> pentru a vedea comenzile disponibile.\n");

        } else if(strncmp(comanda, "download ", 9) == 0 && logat == 1) {

          if(strlen(comanda) == 9) {

            comanda_locala = 1;
            printf("[client]Raspuns : Lipseste argumentul, sintaxa : download <nume>.\n");
          
          } else {

            comanda_locala = 1;
            if(write(sd, comanda, 100) <= 0) {

                  perror("[client]Eroare la write spre server.\n");
                  return errno;
        
            }

            char* raspuns1; int lungime_raspuns1;
            read(sd, &lungime_raspuns1, sizeof(int));
            raspuns1 = (char*)malloc(lungime_raspuns1 + 1);
            read(sd, raspuns1, lungime_raspuns1);
            raspuns1[lungime_raspuns1] = '\0';

            if(strcmp(raspuns1, "Numele introdus nu apartine unui fisier valid de pe server.") == 0 || 
              strcmp(raspuns1, "Eroare la deschidere fisier.") == 0 ||
              strcmp(raspuns1, "Eroare la stat.") == 0 ||
              strcmp(raspuns1, "Eroare la deschidere director curent server.") == 0 ||
              strcmp(raspuns1, "Fisierul nu exista pe server.") == 0) {

                  printf("[client]Raspuns : %s\n", raspuns1);
                  continue;

            }

            char* fisier_download = comanda + 9;
            int nu_exista = 1;
            DIR *director;
            struct dirent *director_curent;
            director = opendir(cale_client);
            if(director) {

              while((director_curent = readdir(director)) != NULL) {

                if(strcmp(fisier_download, director_curent->d_name) == 0) {

                  nu_exista = 0;
                  printf("[client]Raspuns : Exista deja un fisier cu acest nume in directorul local.\n");

                }

              }
              closedir(director);

            } else {

              printf("[client]Eroare la deschidere director curent.\n");
              continue;

            }

            write(sd, &nu_exista, sizeof(int));

            if(!nu_exista) {
              
              continue;

            }

            char cale_fisier_download[500];
            strcpy(cale_fisier_download, cale_client);
            strcat(cale_fisier_download, "/");
            strcat(cale_fisier_download, fisier_download);

            FILE* fisier_nou = fopen(cale_fisier_download, "w");
            if(fisier_nou) {

              long dimensiune_fisier3;
              read(sd, &dimensiune_fisier3, sizeof(long));
              char* continut2 = (char*)malloc(dimensiune_fisier3 + 1);
              read(sd, continut2, dimensiune_fisier3);
              continut2[dimensiune_fisier3] = '\0';
              fwrite(continut2, 1, dimensiune_fisier3, fisier_nou);
              fclose(fisier_nou);
              free(continut2);
              printf("[client]Raspuns : Fisierul a fost descarcat de pe server.\n");
              continue;

            } else {

              printf("[client]Raspuns : Eroare la transfer.\n");
              continue;

            }

          }
          continue;

        } else if(strcmp(comanda, "listc") == 0 && logat == 0) {

          printf("[client]Raspuns : Trebuie sa fii logat pentru a executa aceasta comanda, introdu <help> pentru a vedea comenzile disponibile.\n");
          continue;

        } else if(strcmp(comanda, "listc") == 0 && logat == 1) {

          comanda_locala = 1;
          DIR *director;
          struct dirent *director_curent;
          director = opendir(cale_client);
          char raspuns1[200];
          if(director) {

            strcpy(raspuns1, "Continutul directorului local :");
            while((director_curent = readdir(director)) != NULL) {

                if(strcmp(director_curent->d_name, ".") != 0 && strcmp(director_curent->d_name, "..") != 0) {

                  strcat(raspuns1, "\n");              
                  strcat(raspuns1, director_curent->d_name);
                                
                }

            }
            closedir(director);

          } else {

            strcpy(raspuns1, "Eroare la afisare continut.");

          }

          if(strcmp(raspuns1, "Continutul directorului local :") == 0) {

            strcpy(raspuns1, "Nu exista niciun director sau fisier in ");
              
              strcat(raspuns1, cale_client);
              strcat(raspuns1, ".");

          }

          printf("[client]Raspuns : %s\n", raspuns1);
          continue;

        }

        if(!comanda_locala) {

          // trimiterea comenzii la server 
          if(write(sd, comanda, 100) <= 0) {

            perror("[client]Eroare la write spre server.\n");
            return errno;
        
          }

          // citesc lungimea si raspunsul de la server (apel blocant pina cind serverul raspunde) 

          char* raspuns; int lungime_raspuns;

          if(read(sd, &lungime_raspuns, sizeof(int)) <= 0) {

            perror("[client]Eroare la citirea lungimii raspunsului de la server.\n");
            return errno;

          }

          raspuns = (char*)malloc(lungime_raspuns + 1);

          if(read(sd, raspuns, lungime_raspuns) <= 0) {
      
            perror("[client]Eroare la citirea raspunsului de la server.\n");
            return errno;
        
          }

          raspuns[lungime_raspuns] = '\0';

          if(strcmp(raspuns, "Te-ai logat cu succes.") == 0) {

            logat = 1;

          }

          if(strcmp(raspuns, "Te-ai deconectat cu succes.") == 0) {

            logat = 0;

          } 

          // afisez rezultatul 
          printf("[client]Raspuns : %s\n", raspuns);

          if(strcmp(raspuns, "Te-ai deconectat de la server") == 0) {

            close(sd);
            exit(0);

          }

        }

    }

    // inchidem conexiunea, am terminat 
    close(sd);

}