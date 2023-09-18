#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <WinBase.h>
#include <Lmcons.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define _popen popen
#define _pclose pclose
#define SOCKET int
#define INVALID_SOCKET -1
#define closesocket close
#endif _WIN32
#include <ctime>
#include <algorithm>
#include <cctype> 
#include <cstdio>
#include <cstring>


#define PORT 6666
#define LONGUEUR_MAX 300

std::string execute(const std::string& command)
{
	std::string output = "";

	FILE* pipe = _popen(command.c_str(), "r");
	if (pipe)
	{
		char buffer[LONGUEUR_MAX];
		int lineCount = 0;
		int charCount = 0;
		while (!feof(pipe) && lineCount < 10 && charCount < 300)
		{
			if (fgets(buffer, LONGUEUR_MAX, pipe) != nullptr)
			{
				output += buffer;
				lineCount++;
				charCount += strlen(buffer);
				if (charCount >= 300) {
					output = output.substr(0, output.length() - (charCount - 300));
					break;
				}
			}
		}
		_pclose(pipe);
	}

	if (output.empty())
	{
		output = "OK";
	}

	return output;
}

int main()
{
	bool fermer = true;
	// Boucler la reception du serveur
	while (fermer)
	{
#ifdef _WIN32
		WSADATA wsaData;
		if (WSAStartup(0x0101, &wsaData) != 0)
		{
			std::cout << "Erreur initialisation socket reseau" << std::endl;
			return 1;
		}
#endif _WIN32

		SOCKET sd = socket(AF_INET, SOCK_DGRAM, 0);

		if (sd == INVALID_SOCKET)
		{
			std::cout << "Erreur création socket" << std::endl;
#ifdef _WIN32
			WSACleanup();
#endif _WIN32
			return 1;
		}

		// Création du descripteur du serveur
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = htons(PORT); // Host To Network
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		// Associtation du socket avec le port
		if (bind(sd, (sockaddr*)&addr, sizeof(addr)))
		{
			std::cout << "Erreur bind au port " << std::endl;
			closesocket(sd);
#ifdef _WIN32
			WSACleanup();
#endif _WIN32
			return 1;
		}

#ifdef _WIN32 
		sockaddr_in client;
		char buffer[2048];
		int clientLen = sizeof(client);

		int count = recvfrom(sd, buffer, sizeof(buffer) - 1, 0, (sockaddr*)&client, &clientLen);
#else 
		struct sockaddr_in client;
		char buffer[2048];
		socklen_t clientLen = sizeof(client);

		ssize_t count = recvfrom(sd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&client, &clientLen);
#endif _WIN32

		if (count < 0)
		{
			std::cout << "Erreur reception paquet " << std::endl;
			closesocket(sd);
#ifdef _WIN32
			WSACleanup();
#endif _WIN32			
			return 1;
		}

		buffer[count] = 0; // S'assurer que ca finit avec 0

		std::cout << "RECU: " << buffer << std::endl;

		std::string commande(buffer);

		if (commande.find("echo ") == 0)
		{
			std::string parametre = commande.substr(commande.find(' ') + 1);
			sendto(sd, parametre.c_str(), parametre.length(), 0, (const sockaddr*)&client, sizeof(client));

		}
		else if (commande.find("ping") == 0)
		{
			std::string retour = "pong";
			sendto(sd, retour.c_str(), retour.length(), 0, (const sockaddr*)&client, sizeof(client));
		}
		else if (commande.find("bye") == 0)
		{
			std::string retour = "";
			sendto(sd, retour.c_str(), retour.length(), 0, (const sockaddr*)&client, sizeof(client));

			fermer = false; // Fermer la boucle while

			closesocket(sd); // Fermer le socket
#ifdef _WIN32
			WSACleanup();
#endif _WIN32
			exit(0);
		}
		else if (commande.find("date") == 0)
		{
			std::time_t now = std::time(nullptr);
			std::string retour;
#ifdef _WIN32 
			retour = std::ctime(&now);
#else
			char buffer[80];
			std::strftime(buffer, 80, "%c", std::localtime(&now));
			retour = buffer;
#endif

			sendto(sd, retour.c_str(), retour.length(), 0, (const sockaddr*)&client, sizeof(client));
		}
		else if (commande.find("usager") == 0)
		{

#ifdef _WIN32
			char username[UNLEN + 1];
			DWORD username_len = UNLEN + 1;
			GetUserName(username, &username_len);
			sendto(sd, username, username_len, 0, (const sockaddr*)&client, sizeof(client));
#else
			char username[256 + 1];
			int username_len = 256 + 1;
			getlogin_r(username, username_len);

			sendto(sd, username, username_len, 0, (const struct sockaddr*)&client, sizeof(client));
#endif _WIN32
		}
		else if (commande.find("exec") == 0)
		{

			std::string commandeAExecuter = commande.substr(commande.find(' ') + 1);
			std::string commandeOutput = execute(commandeAExecuter);
			sendto(sd, commandeOutput.c_str(), commandeOutput.length(), 0, (const sockaddr*)&client, sizeof(client));
		}
		else
		{
			sendto(sd, "ERREUR", 6, 0, (const sockaddr*)&client, sizeof(client));
		}
		closesocket(sd); // Fermer le socket
#ifdef _WIN32
		WSACleanup();
#endif _WIN32
	}
}


