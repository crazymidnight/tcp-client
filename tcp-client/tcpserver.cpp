// tcpclient.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <Winsock2.h>

#define SERVICE_PORT 1500

int send_string(SOCKET s, const char * sString);

int main()
{
	SOCKET S;
	SOCKET NS;

	sockaddr_in serv_addr;

	WSADATA wsadata;

	char sName[128];
	bool bTerminate = false;

	// инициализация юиюлиотеки сокетов
	WSAStartup(MAKEWORD(2, 2), &wsadata);

	// попытка получения имени текущей машины
	gethostname(sName, sizeof(sName));
	printf("\nServer host: %s\n", sName);
	
	// создание tcp-сокета 
	if ((S = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
	{
		fprintf(stderr, "Can't create socket\n");
		exit(1);
	}

	// заполняем структуру адресов
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	
	// разрешаем работу на всех доступных сетевых интерфейсах,
	// в частности на localhost
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	// обратите внимание на преобразование порядка байт
	serv_addr.sin_port = htons((u_short)SERVICE_PORT);

	// связываем сокет с заданным сетевым интерфесом и портом
	if (bind(S, (sockaddr*)&serv_addr, sizeof(serv_addr)) == INVALID_SOCKET)
	{
		fprintf(stderr, "Can't bind\n");
		exit(1);
	}

	// перевод сокета в режим прослушивания заданного порта
	// с максимальным количеством ожидания запросов на соединение 5
	if (listen(S, 5) == INVALID_SOCKET)
	{
		fprintf(stderr, "Can't listen\n");
		exit(1);
	}

	printf("Server listen on %s:%d\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));

	// основной цикл обработки подключения клиентов
	while (!bTerminate)
	{
		printf("Wait for connections.....\n");

		sockaddr_in clnt_addr;
		int addrlen = sizeof(clnt_addr);
		memset(&clnt_addr, 0, sizeof(clnt_addr));

		// переводим сервис в режим ожидания запроса на соединение
		// вызов синхронный, т.е. возвращает управление только
		// при подключении клиента или ошибке
		NS = accept(S, (sockaddr*)&clnt_addr, &addrlen);
		if (NS == INVALID_SOCKET)
		{
			fprintf(stderr, "Can't accept connection\n");
			break;
		}

		// получаем параметры присоединенного сокета NS
		// и информации о клиенте
		addrlen = sizeof(serv_addr);
		getsockname(NS, (sockaddr*)&serv_addr, &addrlen);

		// функция inet_ntoa возвращает указатель на глобальный буфер
		// поэтому использовать ее в одном вызове printf не получится
		printf("Accepted connection on %s:%d ",
			inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
		printf("from client %s:%d\n",
			inet_ntoa(clnt_addr.sin_addr), ntohs(clnt_addr.sin_port));
	}

	send_string(NS, "* * * Welcome to simple UPCASE TCP-server * * *\r\n");
	char sReceiveBuffer[1024] = { 0 };
	
	// получаем и обрабатываем данные от клиента
	while (true)
	{
		int nReaded = recv(NS, sReceiveBuffer, sizeof(sReceiveBuffer) - 1, 0);
		
		// в случае ошибки (например, отсоединение клиента) выходи
		if (nReaded <= 0) break;
		
		// получаем поток байт, поэтому нужно самостоятельно
		// добавить завершающий 0 для ASCII строки
		sReceiveBuffer[nReaded] = 0;

		// отбрасываем символы перевода строк
		for (char* pPtr = sReceiveBuffer; *pPtr != 0; pPtr++)
		{
			if (*pPtr == '\n' || *pPtr == '\r')
			{
				*pPtr = 0;
				break;
			}
		}

		// пропускаем пустые строки
		if (sReceiveBuffer[0] == 0) continue;

		printf("Received data: %s\n", sReceiveBuffer);

		// анализируем полученные команды или преобразуем текст в верхний регистр
		if (strcmp(sReceiveBuffer, "info") == 0)
		{
			send_string(NS, "Test TCP-server.\r\n");
		}
		else if (strcmp(sReceiveBuffer, "exit") == 0)
		{
			send_string(NS, "Bye...\r\n");
			printf("Client initialize disconnection.\r\n");
			break;
		}
		else if (strcmp(sReceiveBuffer, "shutdown") == 0)
		{
			send_string(NS, "Server go to shutdown.\r\n");
			Sleep(200);
			bTerminate = true;
			break;
		}
		else
		{
			// преобразуем строку в верхний регистр
			char sSendBuffer[1024];
			_snprintf(sSendBuffer, sizeof(sSendBuffer), "Server reply: %s\r\n",
				_strupr(sReceiveBuffer));
			send_string(NS, sSendBuffer);
		}
	}

	// закрываем серверный сокет
	closesocket(S);

	// освобождаем ресурсы библиотеки сокетов
	WSACleanup();
	
	return 0;
}

int send_string(SOCKET s, const char * sString)
{
	return send(s, sString, strlen(sString), 0);
}