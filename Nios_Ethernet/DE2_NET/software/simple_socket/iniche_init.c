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

#define HTTP_PORT        80
#define HTTP_MAX_MSG_LEN 100 //limite de caracteres especificado no arquivo
#define HTTP_RX_BUF_SIZE 2048 //  Tamano do buffer para caber o cabeçario http + o corpo


/*Estrutura para compartilhar mensagens HTTP entre os dois*/

typedef struct
{
	int sock; //ID do socket que vai ficar conectado ao navegador
	char texto[HTTP_MAX_MSG_LEN + 1]; //String que vai ser extraida do corpo HTTP(considerando o \0)
	int len; //tamanho da mensagem
}HttpMsg;


OS_EVENT *HttpReqQ; //ponteiro para a fila de mensagem do Micro/OS
void *HttpReqQTbl[4]; // Vetor de inteira da fila(para guardar 4 mensagens)
OS_EVENT *HttpSlotSem; //Envio que serv para garantir que o RX não sobrescreve antes do TX respoder



static HttpMsg req_atual; //Estático, a mensa mensagem na thread


//Macros obrigatórias do NicheStack para declarar uma thread que usa a rede
TK_OBJECT(to_rxtask); //Objeto de controle da tarefa na NicheStack
TK_ENTRY(HttpRxTask); //Declaração do protótipo: int HttpRxTask(int parm)




/*THREAD do RX*/
struct inet_taskinfo rxtask = 
{
&to_rxtask, //Endereço de objeto
"http rx task", //nome da thread
HttpRxTask, //Função que ela vai executar
8, // Prioridade (8 porque as outras estão sendo usadas no sistema)
APP_STACK_SIZE, //Tamanho da pilha da thread
NULL //Poteiro base da pilha
}




int HttpRxTask (int parm)
{
	int sock_escuta
	int cli_sock;
	int cli_len;
	struct sockaddr_in servidor_addr;
	struck sockaddr_in cli_addr;
	INT8U erro; // para guardar código de erro das funções do MicroC/OS


	//Buffer estático onde o recv() vai jogar os bytes vindo da placa
	static char rx_buf[HTTP_RX_BUF_SIZE];

	//Variáveis exigida pelo picohttpparsec
	struct phr_header headers[16]; //guarda cabeçarios
	const char *method; //Aponta para o comando (post, get)
	const char *path // aponta para a url no buffer
	size_t method_len,path_len; // tamanho de bytes do método e da URL
	size_t num_headers; // Entra no máximo(16) e sai quantos vieres
	int http_ver; // versão do HTTP


	int total; //total de buutes acumulados no rx buff
	int prev_len; //qtd de bytes antes da última leitura
	int pret; //retorno do picohttpparser
	int r; //quantidade de bytes lidos em uma chamada
	int content_len; //Tamano do corpo lido do header
	size_t i;



	//CRIAÇÃO DO SOCKET TCP
	fd = socket(AF_INET,SOCK-STREAM,0); //cria socket TCP IPv4
	if(fd_lsiten <0)
	{
		printf("[RX] Erro ao criar socket\n")
		TK_RETURN_ERROR();
	}

	//Configurando a porta 80 para eschuta
	memset(&servidor_addr,0,sizeof(servidor_addr));
	servidor_addr.sin_family = AF_INET;
	servidor_addr.sin_port = htons(HTTP_PORT);
	servidor_addr.sin_addr.sin_addr = INADDR_ANY;

	//fazendo o bind do socket na porta 80 da placa
	if(bind(fd_listen,(struct sockaddr *)&server_addr,sizeof(server_addr)) < 0)
	{
		TK_RETURN_ERROR();
	}

	//Colocando o socket em modo de escuta
	if(listen(fd_listen,4) < 0)
	{
		TK_RETURN_ERROR();
	}


	printf("RX Servidor HTTP escutando em: %d",HTTP_PORT);

	//Loop infito da thread para receber uma requisição
	while(1)
	{
		cli_len = sizeof(cli_addr);



		//o accept bloqueia a cpu(faz ela dormir) até o PC se comunicar com ele
		cli_sock = accept(fd_listen, (struct sockaddr *)&cli_addr, &cli_len);

		if(cli_sock < 0)
		{
			TK_SLEEP(1);
			continue;
		}


		//Contadores pra ler a requisição HTTP
		total = 0;
		prev_len = 0;
		pret = -1;

		//Loop de leitura do cabeçalho HTTP
		while(1)
		{
			//Lê bytes enviados pelo navegador e acumula em rx_buf + total
			r = recv(cli_sock,rx_buf + total, sizeof(rx_buf) - total - 1,0);
			if(r<=0)
			{
				pret = -1;
				break;
			}

			pret
		}

	}





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
  LCD_Init();
  int sw, but;
  char Text[16] = "Requisitando aq";
  LCD_Show_Text(Text);
  char Text00[16] = "arquivoA.txt   ";
  char Text01[16] = "arquivoB.txt   ";
  char Text10[16] = "arquivoC.txt   ";
  char Text11[16] = "invalido.txt   ";
  char ArqI[32] = "ARQ INVALIDO !!!ARQ INVALIDO !!!";
  char p1[16];
  char p2[16];
  char flag = 0; //0-> escolhendo, 1->scroll
  char lastbut = 0x0F;
  unsigned int linha1=0;
  unsigned int linha2=16;
  char choice = 0;
  struct sockaddr_in sa;
  int res;
  int SocketFD;
  char reqA[6]= "queroa";
  char reqB[6]= "querob";
  char reqC[6]= "queroc";
  char buf[2000];
  printf("estou aqui\n");
  SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  printf("Socket criado\n");
  memset(&sa, 0, sizeof sa);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(7777); // ALTERAR PORTA A SER UTILIZADA AQUI
  res = inet_pton(AF_INET, "192.168.137.1", &sa.sin_addr); //ALTERAR O IP DO SERVIDOR AQUI
  if (connect(SocketFD, (struct sockaddr *)&sa, sizeof sa) == -1) {
	perror("connect failed");
	close(SocketFD);
	exit(EXIT_FAILURE);
  }
  while (1){
	  	/*
	    if (send(SocketFD, buf, sizeof(buf), 0) < 0) //exemplo de envio
	    {
	        perror("Send()");
	        exit(EXIT_FAILURE);
	    }else{
	    	printf("Msg enviada: %s\n", buf);
	    }
	    if (recv(SocketFD, buf, sizeof(buf), 0) < 0) //exemplo de recebimento
	    {
	        perror("Recv()");
	        exit(EXIT_FAILURE);
	    }else{
	    	printf("Msg recebida: %s\n", buf);
	    	memcpy(buf, original, 12 * sizeof(char));
	    }
	    msleep(1000);*/
	  	sw = IORD_ALTERA_AVALON_PIO_DATA(SWITCH_PIO_BASE) & 0x03;
		but = IORD_ALTERA_AVALON_PIO_DATA(BUTTON_PIO_BASE) & 0x0F;

		  	if(flag == 0) // estado de escolha
		  	{
		  		if(sw==0){LCD_Line2(); LCD_Show_Text(Text00);}
		  		if(sw==1){LCD_Line2(); LCD_Show_Text(Text01);}
		  		if(sw==2){LCD_Line2(); LCD_Show_Text(Text10);}
		  		if(sw==3){LCD_Line2(); LCD_Show_Text(Text11);}
		  		if(but == 13 && but != lastbut)
		  		{
		  			flag = 1;
		  			if(sw == 0)
		  			{
		  				choice = 1;
		  			    if (send(SocketFD, reqA, sizeof(reqA), 0) < 0) //exemplo de envio
		  			    {
		  			        perror("Send()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg enviada: queroa\n");
		  			    }
		  			    if (recv(SocketFD, buf, sizeof(buf), 0) < 0) //exemplo de recebimento
		  			    {
		  			        perror("Recv()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg recebida: %s\n", buf);
		  			    	memcpy(p1, buf, 16 * sizeof(char));
		  			    	memcpy(p2, &buf[16], 16 * sizeof(char));
		  			    }
		  			}else if(sw == 1)
		  			{
		  				choice = 2;
		  			    if (send(SocketFD, reqB, sizeof(reqB), 0) < 0) //exemplo de envio
		  			    {
		  			        perror("Send()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg enviada: querob\n");
		  			    }
		  			    if (recv(SocketFD, buf, sizeof(buf), 0) < 0) //exemplo de recebimento
		  			    {
		  			        perror("Recv()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg recebida: %s\n", buf);
		  			    	memcpy(p1, buf, 16 * sizeof(char));
		  			    	memcpy(p2, &buf[16], 16 * sizeof(char));
		  			    }
		  			}else if(sw == 2)
		  			{
		  				//TransmitPacket(arq3,0x06);
		  				choice = 3;
		  			    if (send(SocketFD, reqC, sizeof(reqC), 0) < 0) //exemplo de envio
		  			    {
		  			        perror("Send()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg enviada: queroc\n");
		  			    }
		  			    if (recv(SocketFD, buf, sizeof(buf), 0) < 0) //exemplo de recebimento
		  			    {
		  			        perror("Recv()");
		  			        exit(EXIT_FAILURE);
		  			    }else{
		  			    	printf("Msg recebida: %s\n", buf);
		  			    	memcpy(p1, buf, 16 * sizeof(char));
		  			    	memcpy(p2, &buf[16], 16 * sizeof(char));
		  			    }
		  			}else if(sw == 3)
		  			{
		  				//TransmitPacket(arq4,0x06);
		  				choice = 4;
		  				memcpy(p1, ArqI, 16 * sizeof(char));
		  				memcpy(p2, &ArqI[16], 16 * sizeof(char));
		  			}
		  		}
		  	}else if(flag == 1) // scroll
		  	{
		  		LCD_Line1();
		  		LCD_Show_Text(p1);
		  		LCD_Line2();
		  		LCD_Show_Text(p2);
		  		// if dos botoes
		  		if(but == 11 && but != lastbut)
		  		{
		  			if(linha2<1984){
		  				linha1+=16;
		  				linha2+=16;

		  			}
		  		}else if(but == 7 && but != lastbut)
		  		{
		  			if(linha1>=16){
		  				linha1-=16;
		  				linha2-=16;
		  			}
		  		}else if(but==13 && but!=lastbut)
		  		{
		  			flag = 0;
		  			linha1 = 0;
		  			linha2 = 16;
		  			LCD_Clear();
		  			LCD_Show_Text(Text);
		  		}

		  		if (choice != 4){
		  			memcpy(p1, &buf[linha1], 16 * sizeof(char));
			    	memcpy(p2, &buf[linha2], 16 * sizeof(char));
		  		}

		  	}

		  	lastbut=but;

	    msleep(100);
  }
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
