/******************************************************************************
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
* All rights reserved. All use of this software and documentation is          *
* subject to the License Agreement located at the end of this file below.     *
*******************************************************************************                                                                             *
* Date - October 24, 2006                                                     *
* Module - iniche_init.c                                                      *
*                                                                             *                                                                             *
******************************************************************************/

/******************************************************************************
 * NicheStack TCP/IP stack initialization and Operating System Start in main()
 * for Simple Socket Server (SSS) example. 
 * 
 * This example demonstrates the use of MicroC/OS-II running on NIOS II.       
 * In addition it is to serve as a good starting point for designs using       
 * MicroC/OS-II and Altera NicheStack TCP/IP Stack - NIOS II Edition.                                                                                           
 *      
 * Please refer to the Altera NicheStack Tutorial documentation for details on 
 * this software example, as well as details on how to configure the NicheStack
 * TCP/IP networking stack and MicroC/OS-II Real-Time Operating System.  
 */
  
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <io.h>
#include <fcntl.h>
#include "picohttpparser.h"
/* MicroC/OS-II definitions */
#include "../simple_socket_bsp/HAL/inc/includes.h"

#include "../simple_socket_bsp/system.h"

#include "dm9000a.h"

/* Simple Socket Server definitions */
#include "simple_socket_server.h"
#include "alt_error_handler.h"

/* Nichestack definitions */
#include "../simple_socket_bsp/iniche/src/h/nios2/ipport.h"
#include "../simple_socket_bsp/iniche/src/h/tcpport.h"
#include "../simple_socket_bsp/iniche/src/h/libport.h"
#include "../simple_socket_bsp/iniche/src/nios2/osport.h"
#include "basic_io.h"
#include "LCD.h"
#include "altera_avalon_pio_regs.h"
/* Definition of task stack for the initial task which will initialize the NicheStack
 * TCP/IP Stack and then initialize the rest of the Simple Socket Server example tasks. 
 */
OS_STK    SSSInitialTaskStk[TASK_STACKSIZE];

/* Declarations for creating a task with TK_NEWTASK.  
 * All tasks which use NicheStack (those that use sockets) must be created this way.
 * TK_OBJECT macro creates the static task object used by NicheStack during operation.
 * TK_ENTRY macro corresponds to the entry point, or defined function name, of the task.
 * inet_taskinfo is the structure used by TK_NEWTASK to create the task.
 */
TK_OBJECT(to_ssstask);
TK_ENTRY(SSSSimpleSocketServerTask);

struct inet_taskinfo ssstask = {
      &to_ssstask,
      "simple socket server",
      SSSSimpleSocketServerTask,
      4,
      APP_STACK_SIZE,
};


//DESCRIÇÃO DAS UNIDADES

#define HTTP_PORT        80    /* Porta padrão do protocolo HTTP (http://IP/) */
#define HTTP_MAX_MSG_LEN 100   /* Limite máximo de 100 caracteres pedido no espec.txt */
#define HTTP_RX_BUF_SIZE 2048  /* Tamanho do buffer para caber cabeçalho HTTP + body */
#define HTTP_TX_BUF_SIZE 512   /* Tamanho do buffer para montar a resposta HTTP */

/* Estrutura que empacota os dados que a Thread RX vai enviar para a Thread TX */
typedef struct {
    int  sock;                         /* ID do socket conectado com o navegador (para a TX responder nele) */
    char texto[HTTP_MAX_MSG_LEN + 1];  /* String extraída do body HTTP (+1 para o '\0' final) */
    int  len;                          /* Quantidade de caracteres da mensagem */
} HttpMsg;

/* Recursos do sistema operacional MicroC/OS-II para comunicação entre as 2 threads */
OS_EVENT *HttpReqQ;            /* Ponteiro para a Fila de Mensagens (Queue) do MicroC/OS-II */
void     *HttpReqQTbl[4];      /* Vetor interno da fila (guarda até 4 ponteiros de mensagens) */
OS_EVENT *HttpSlotSem;         /* Semáforo binário: evita que a RX sobrescreva 'req_atual' antes da TX terminar */

/* Variável estática (global): NUNCA coloque na stack local da função, pois a outra thread vai ler este endereço! */
static HttpMsg req_atual;

/* Macros obrigatórias da NicheStack para declarar uma thread que usa rede (sockets) */
TK_OBJECT(to_rxtask);          /* Cria o objeto interno de controle da task na NicheStack */
TK_ENTRY(HttpRxTask);          /* Declara o protótipo: int HttpRxTask(int parm) */

/* Estrutura de configuração da Thread RX passada para o TK_NEWTASK */
struct inet_taskinfo rxtask = {
    &to_rxtask,                /* Endereço do objeto da task */
    "http rx task",            /* Nome da thread (aparece no console quando é criada) */
    HttpRxTask,                /* Função que a thread vai executar */
    8,                         /* Prioridade no MicroC/OS-II (única! 1, 2, 3 e 5 já são usadas pelo sistema) */
    APP_STACK_SIZE,            /* Tamanho da pilha da thread (~14 KB definidos no ipport.h) */
    NULL                       /* Ponteiro base da pilha (preenchido automaticamente pelo TK_NEWTASK) */
};

 // THREAD DE RECEPÇÃO HTTP (Task RX)

void HttpRxTask(void *parm) {
    int fd_listen;                 /* Socket "ouvinte" (servidor) que fica esperando conexões na porta 80 */
    int cli_sock;                  /* Socket específico do cliente que acabou de conectar */
    int cli_len;                   /* Tamanho da struct de endereço do cliente */
    struct sockaddr_in srv_addr;   /* Endereço/porta onde o servidor vai escutar */
    struct sockaddr_in cli_addr;   /* Endereço IP/porta de quem conectou (navegador do PC) */
    INT8U err;                     /* Guarda código de erro das funções do MicroC/OS-II */

    /* Buffer estático onde o recv() joga os bytes crus vindos da placa de rede */
    static char rx_buf[HTTP_RX_BUF_SIZE];
    char cl_str[16];               /* Buffer auxiliar seguro para extrair o valor de Content-Length */

    /* Variáveis exigidas pelo picohttpparser para devolver os pedaços do cabeçalho HTTP */
    struct phr_header headers[32]; /* Guarda até 16 cabeçalhos (ex: Host, Content-Length, etc.) */
    const char *method;            /* Apontará para o método no buffer (ex: "POST" ou "GET") */
    const char *path;              /* Apontará para a URL no buffer (ex: "/") */
    size_t method_len, path_len;   /* Tamanho em bytes do método e da URL */
    size_t num_headers;            /* Entra com o máximo (16) e sai com quantos headers vieram */
    int minor_version;             /* Versão do HTTP (1 para HTTP/1.1) */

    int total;                     /* Total de bytes acumulados no rx_buf até agora */
    int prev_len;                  /* Quantos bytes tínhamos antes da última leitura recv() */
    int pret;                      /* Retorno do picohttpparser (>0: tamanho do header, -2: incompleto, -1: erro) */
    int r;                         /* Quantidade de bytes lidos em uma chamada recv() */
    int content_len;               /* Tamanho do corpo (body) lido do header Content-Length */
    size_t copy_len;
    size_t i;

    //Criando um socket IPv4
    fd_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_listen < 0) {
        printf("[RX] Erro ao criar socket\n");
        TK_RETURN_ERROR();
    }

    //Configuração da porta IP
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(HTTP_PORT);
    srv_addr.sin_addr.s_addr = INADDR_ANY;

    //Bind com a porta 80
    if (bind(fd_listen, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) < 0) {
        printf("[RX] Erro no bind\n");
        close(fd_listen);
        TK_RETURN_ERROR();
    }

    //Coloca no modo fila para receber as conexões
    if (listen(fd_listen, 4) < 0) {
        printf("[RX] Erro no listen\n");
        close(fd_listen);
        TK_RETURN_ERROR();
    }

    printf("[RX] Servidor HTTP escutando na porta %d\n", HTTP_PORT);

    //Loop de recepção dos pacotes
    while (1) {
        cli_len = sizeof(cli_addr);

		//o accept bloqueia a thread até chegar um pacote
        cli_sock = accept(fd_listen, (struct sockaddr *)&cli_addr, &cli_len);
        if (cli_sock < 0) {
            TK_SLEEP(1);
            continue;
        }

        //Zera as váriáveis do picohttpparser para ler o cabeçario http
        total = 0;
        prev_len = 0;
        pret = -1;

        /* Loop de leitura do cabeçalho HTTP */
        while (1) {
            //Lê os bytes que a rede vai enviado, que pode ser separado
            r = recv(cli_sock, rx_buf + total, sizeof(rx_buf) - total - 1, 0); 
            if (r <= 0) {
                pret = -1; /* Conexão fechou ou deu erro */
                break;
            }
            prev_len = total; //guarda a última parte de bytes recebido
            total += r; //guarda o total de bytes recebido
            rx_buf[total] = '\0'; //como medida de segurança, colo fim da string no final da leitura

            //manda os bytes bruto para o picoparser ler a requisição
            num_headers = 32; //número que serve para saber quantos headers tera o pacote HTTP
            pret = phr_parse_request(rx_buf, (size_t)total,
                                     &method, &method_len,
                                     &path, &path_len,
                                     &minor_version,
                                     headers, &num_headers, (size_t)prev_len);

            if (pret > 0) break;   //Cabeçario terminou
            if (pret == -1) break; //Erro de formato HTTP
            if (total >= (int)sizeof(rx_buf) - 1) {
                pret = -1;         //buffer estotou sem terminar o cabeçario
                break;
            }
          //Continua o loop para pegar todos os bytes do HTTP
        }

        //Faz um double check e confere se o picoparser deu certo e se o que o http enviou foi POST
        if (pret > 0 && method_len == 4 && memcmp(method, "POST", 4) == 0) {

            //Procura no cabeçario do Content-Lenght o texto certo
            content_len = 0;
            for (i = 0; i < num_headers; i++) {
				//Loop para ver se primeiro tem 14 bytes, segundo para compara os nomes e ver se são iguais(14 bytes)
                if (headers[i].name_len == 14 && strncasecmp(headers[i].name, "Content-Length", 14) == 0) {
                    
                    //garante que eu copie os símboltos certos e termine em 0/
                    copy_len = headers[i].value_len < (sizeof(cl_str) - 1) //<15 
                               ? headers[i].value_len //tamanho certo
                               : (sizeof(cl_str) - 1); //tamanho padrão
                    memcpy(cl_str, headers[i].value, copy_len); //compia o que tem dentro do header para fazer a string
                    cl_str[copy_len] = '\0'; // adiciona o terminador final
                    content_len = atoi(cl_str); // copia para a memória como int
                    break;
                }
            }

            //proteção de dados
            if (content_len < 0) {
                content_len = 0;
            } else if (content_len > HTTP_MAX_MSG_LEN) {
                content_len = HTTP_MAX_MSG_LEN;
            }
			

			//loop de proteção para garantir todos os dados, já que o BODY pode ter ficado em outro apcote
            while ((total - pret) < content_len) {
                r = recv(cli_sock, rx_buf + total, sizeof(rx_buf) - total - 1, 0);
                if (r <= 0) break;
                total += r;
            }

            //ajusta a conta para caso ocorra uma queda de conexão
            if ((total - pret) < content_len) {
                content_len = total - pret;
            }

            //Espera o semáforo ficar pronto
            OSSemPend(HttpSlotSem, 0, &err);

            //Preenche toda a estrutura que será lida pela thread do TX
            req_atual.sock = cli_sock;
            req_atual.len  = content_len;
            /* Como 'pret' é o tamanho exato do cabeçalho, 'rx_buf + pret' aponta direto para o 1º byte do body! */
            memcpy(req_atual.texto, rx_buf + pret, content_len);
            req_atual.texto[content_len] = '\0';

            printf("[RX] Mensagem recebida (%d chars): %s\n", req_atual.len, req_atual.texto);

            //envia a requisição atual para a ThreadRX
            OSQPost(HttpReqQ, (void *)&req_atual);

        } else {
            close(cli_sock);
        }
    }

    TK_RETURN_OK();
}


//Definição da thread de recepção


/* Macros obrigatórias da NicheStack para declarar a Thread TX */
TK_OBJECT(to_txtask);          /* Objeto interno de controle da task TX */
TK_ENTRY(HttpTxTask);          /* Protótipo: int HttpTxTask(int parm) */

/* Estrutura de configuração da Thread TX passada para o TK_NEWTASK */
struct inet_taskinfo txtask = {
    &to_txtask,                /* Endereço do objeto da task */
    "http tx task",            /* Nome da thread */
    HttpTxTask,                /* Função que a thread vai executar */
    9,                         /* Prioridade única no MicroC/OS-II (diferente da RX que é 8!) */
    APP_STACK_SIZE,            /* Tamanho da pilha da thread */
    NULL                       /* Ponteiro base da pilha */
};

#include "io.h"       /* Necessário para IORD e IOWR no Nios II */
#include "system.h"   /* Onde fica definido o USER_HW_BASE gerado pelo Qsys */

/* Exemplo de offsets dos registradores do seu user_hw (ajuste conforme seu Verilog/VHDL) */
#define ESCREVER  0   
#define LER   1   
#define DADO_ESCRITA  2   /* Entrada de dados para o hardware */
#define DADO_LEITURA 3   /* Saída processada pelo hardware */

//ThreaRX

void HttpTxTask(void *parm) {
    INT8U err;                         /* Código de erro retornado pelo MicroC/OS-II */
    HttpMsg *msg;                      /* Ponteiro que receberá o endereço de 'req_atual' */
    static char tx_buf[HTTP_TX_BUF_SIZE]; /* Buffer para montar o cabeçalho + corpo da resposta */
    char body_buf[160];                /* Buffer auxiliar para montar o texto do corpo da resposta */
    int body_len;                      /* Tamanho do corpo em bytes */
    int total_len;                     /* Tamanho total (cabeçalho + corpo) a enviar */
    int sent;                          /* Quantos bytes já foram enviados pelo send() */
    int r;                             /* Retorno de cada chamada send() */

    printf("[TX] Thread de resposta HTTP iniciada\n");

    while (1) {
        //Dorme até receber uma mensagem
        msg = (HttpMsg *)OSQPend(HttpReqQ, 0, &err);

        //Verifica se a leitura da fila deu certo
        if (err != OS_NO_ERR || msg == NULL) {
            continue;
        }

        printf("[TX] Processando resposta para mensagem: \"%s\"\n", msg->texto);

        
		//interação com o user hardware
        char hw_resposta[HTTP_MAX_MSG_LEN + 1];
        int j;

        printf("[TX] Enviando \"%s\" para o user_hw...\n", msg->texto);

        //Loop para enviar os dados para o user hardware
        for (j = 0; j < msg->len; j++) {
           //Escreve no caractere
        	IOWR(USER_HW_0_BASE, 0, (alt_u32)(unsigned char)msg->texto[j]); //conversão para a comparação funcionar em hardware

          
			//le os resultados
            hw_resposta[j] = (char)IORD(USER_HW_0_BASE, 0);
        }
        hw_resposta[msg->len] = '\0'; //Fecha a string

        printf("[TX] Resposta do user_hw: \"%s\"\n", hw_resposta);

        //preparação para a resposta
        body_len = snprintf(body_buf, sizeof(body_buf), "%s", hw_resposta);

        if (body_len < 0) body_len = 0;
        if (body_len >= (int)sizeof(body_buf)) body_len = sizeof(body_buf) - 1;

        //monta o cabeçario htpp da resposta
        total_len = snprintf(tx_buf, sizeof(tx_buf),
                             "HTTP/1.1 200 OK\r\n"
                             "Content-Type: text/plain; charset=utf-8\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Content-Length: %d\r\n"
                             "Connection: close\r\n"
                             "\r\n"
                             "%s",
                             body_len, body_buf);


        if (total_len < 0) total_len = 0;
        if (total_len >= (int)sizeof(tx_buf)) total_len = sizeof(tx_buf) - 1;

       //envia para o cliente(pc)
        sent = 0;
        while (sent < total_len) {
            r = send(msg->sock, tx_buf + sent, total_len - sent, 0);
            if (r <= 0) {
                printf("[TX] Erro ao enviar resposta HTTP\n");
                break;
            }
            sent += r;
        }

        //Fecha a conexão http
        close(msg->sock);

        printf("[TX] Resposta enviada (%d bytes) e socket fechado.\n", sent);

        //libera o semáforo
        OSSemPost(HttpSlotSem);
    }

    TK_RETURN_OK();
}





/* SSSInitialTask will initialize the NicheStack
 * TCP/IP Stack and then initialize the rest of the Simple Socket Server example 
 * RTOS structures and tasks. 
 */
void SSSInitialTask(void *task_data)
{
  INT8U error_code;
  
  /*
   * Initialize Altera NicheStack TCP/IP Stack - Nios II Edition specific code.
   * NicheStack is initialized from a task, so that RTOS will have started, and 
   * I/O drivers are available.  Two tasks are created:
   *    "Inet main"  task with priority 2
   *    "clock tick" task with priority 3
   */   
  alt_iniche_init();
  netmain(); 

  /* Wait for the network stack to be ready before proceeding. 
   * iniche_net_ready indicates that TCP/IP stack is ready, and IP address is obtained.
   */
  while (!iniche_net_ready)
    TK_SLEEP(1);

  /* Now that the stack is running, perform the application initialization steps */
  
  /* Application Specific Task Launching Code Block Begin */

  printf("\nSimple Socket Server starting up\n");

  /* Create the main simple socket server task. */
  //TK_NEWTASK(&ssstask);
  
  /*create os data structures */
  //SSSCreateOSDataStructs();

  /* create the other tasks */
  //SSSCreateTasks();

  /* Application Specific Task Launching Code Block End */
  
  /*This task is deleted because there is no need for it to run again */
  //error_code = OSTaskDel(OS_PRIO_SELF);
  //alt_uCOSIIErrorHandler(error_code, 0);


  //Trecho para iniciar o servidor
    printf("\nSimple Socket Server starting up\n");

    //cria a fila de mensagem
    HttpReqQ = OSQCreate(&HttpReqQTbl[0], 4);

    
	//cria o semáforo
    HttpSlotSem = OSSemCreate(1);

    //Cria e inicia ambas as threads
    TK_NEWTASK(&rxtask);
    TK_NEWTASK(&txtask);

    error_code = OSTaskDel(OS_PRIO_SELF);
    alt_uCOSIIErrorHandler(error_code, 0);
}

/* Main creates a single task, SSSInitialTask, and starts task scheduler.
 */

int main (int argc, char* argv[], char* envp[])
{
  
  INT8U error_code;

  DM9000A_INSTANCE( DM9000A_0, dm9000a_0 );
  DM9000A_INIT( DM9000A_0, dm9000a_0 );

  /* Clear the RTOS timer */
  OSTimeSet(0);

  /* SSSInitialTask will initialize the NicheStack
   * TCP/IP Stack and then initialize the rest of the Simple Socket Server example
   * RTOS structures and tasks.
   */
  error_code = OSTaskCreateExt(SSSInitialTask,
                             NULL,
                             (void *)&SSSInitialTaskStk[TASK_STACKSIZE],
                             SSS_INITIAL_TASK_PRIORITY,
                             SSS_INITIAL_TASK_PRIORITY,
                             SSSInitialTaskStk,
                             TASK_STACKSIZE,
                             NULL,
                             0);
  alt_uCOSIIErrorHandler(error_code, 0);

  /*
   * As with all MicroC/OS-II designs, once the initial thread(s) and
   * associated RTOS resources are declared, we start the RTOS. That's it!
   */
  OSStart();
  
  while(1); /* Correct Program Flow never gets here. */

  return -1;
}

/******************************************************************************
*                                                                             *
* License Agreement                                                           *
*                                                                             *
* Copyright (c) 2006 Altera Corporation, San Jose, California, USA.           *
* All rights reserved.                                                        *
*                                                                             *
* Permission is hereby granted, free of charge, to any person obtaining a     *
* copy of this software and associated documentation files (the "Software"),  *
* to deal in the Software without restriction, including without limitation   *
* the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
* and/or sell copies of the Software, and to permit persons to whom the       *
* Software is furnished to do so, subject to the following conditions:        *
*                                                                             *
* The above copyright notice and this permission notice shall be included in  *
* all copies or substantial portions of the Software.                         *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
* DEALINGS IN THE SOFTWARE.                                                   *
*                                                                             *
* This agreement shall be governed in all respects by the laws of the State   *
* of California and by the laws of the United States of America.              *
* Altera does not recommend, suggest or require that this reference design    *
* file be used in conjunction or combination with any other product.          *
******************************************************************************/
