//
//  test.cpp
//  
//
//  Created by Sylvain Blocquaux on 26/03/15.
//
// g++ -Wall -o test test.cpp

#include <iostream>
#include <termios.h>
#include <sys/fcntl.h> // open()
#include <stdlib.h> // exit()
#include <fstream> // Ouverture du fichier texte
#include <string>
#include <sstream> // stringstream
#include <map>

using namespace std;

bool parite(char b) {
    b ^= b >> 4;
    b ^= b >> 2;
    b ^= b >> 1;
    return b & 1;
}

char* nomFichier(int ep, int ind) {
	stringstream nf;
	nf << ep << "/" << ep;
	if (ind < 10) {
		nf << "0" << ind;
	} else {
		nf << ind;
	}
	return (char*)nf.str().c_str();
}

void affichePage(int fd, string pageCourante) {
	char* memblock;
	ifstream fichier(pageCourante.c_str(), ios::in|ios::binary|ios::ate);
	if (fichier.is_open()){
		cout << "Fichier ouvert" << endl;
		streampos size = fichier.tellg();
		memblock = new char[size];
		fichier.seekg (0, ios::beg);
    	fichier.read (memblock, size);
		fichier.close();
		
		for (unsigned int i = 0; i < size; i++) { // Assigne le bit de parité
			if (parite(memblock[i])) {
				memblock[i] |= (1 << 7);
			} else {
				memblock[i] &= ~(1 << 7);
			}
		}
		write(fd, memblock, size);
		tcdrain(fd); // Attend la fin
		cout << "Fin de la séquence" << endl;
	}
	tcflush(fd, TCIOFLUSH); // Vide les tampons du port série
}

int main(void) {
	cout << "Processus lancé" << endl;
	int fd = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY);
	if (fd == -1) {
		cout << "Erreur lors de l'ouverture du port série" << endl;
		exit(EXIT_FAILURE);
	}
	struct termios term;
	tcgetattr(fd, &term);
	term.c_iflag = IGNPAR;
	term.c_oflag = 0;
	term.c_cflag = CS8 | B1200;
	term.c_lflag = 0;
	tcsetattr(fd, TCSANOW, &term);
	// Annule l'echo local empêche le modem d'écrire sur l'écran
	char a[] = {0x0C, 0X1B, 0x3B, 0X60, 0x58, 0x52};
	for (int i = 0; i < 8; i++) {
		if (parite(a[i])) {
			a[i] |= (1 << 7);
		} else {
			a[i] &= ~(1 << 7);
		}
	}
	write(fd, &a, 8);
	string page("0/Menu.vdt");
	map<string, string> commandes;
	while (1) {
		affichePage(fd, page);
		string commande(page);
		commande.replace(commande.find(".vdt"), 4, ".cmd");
		commandes.clear();
		ifstream fichierCommandes(commande.c_str());
		string ligne;
		while (getline (fichierCommandes,ligne)){
			commandes[ligne.substr(0, ligne.find(' '))] = ligne.substr(ligne.find(' ') + 1);
    	}
    	fichierCommandes.close();
		bool fonction = false;
		string com;
		do {
			unsigned char tampon[1];
			read(fd, tampon, sizeof(tampon));
			tampon[0] &= ~(1 << 7); // Enlève le bit de parité
			if (tampon[0] == 0x13) {
				fonction = true;
			} else {
				if (fonction) {
					fonction = false;
					//printf ("%02x ", tampon[0]);
					switch (tampon[0]) {
						case 0x41 : com = "Envoi";
								break;
						case 0x42 : com = "Retour";
								break;
						case 0x46 : com = "Sommaire";
								break;
						case 0x48 : com = "Suite";
								break;
						default : com = "";
					}
				} else {
					com = tampon[0];
				}
				//cout << commandes[com] << endl;
			}
		} while(!commandes[com].compare(""));
		cout << commandes[com] << endl;
		page.assign(commandes[com]);
	}	// Bye...
	close(fd);
	return 0;
}
