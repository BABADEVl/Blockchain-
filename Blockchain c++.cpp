#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>

const int WIDTH = 60;
std::atomic<bool> found(false);  // Variable atomique pour signaler quand un nonce valide est trouv�
std::mutex outputMutex;  // Mutex pour �viter les conflits d'affichage

// Fonction pour calculer le hash en fonction de l'index, du precedent hash, des donnees et du timestamp
std::string calculateHash(int index, const std::string& previousHash, const std::string& data, const std::tm& time, int nonce)
{
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << index << previousHash << data << std::put_time(&time, "%Y-%m-%d %H:%M:%S") << nonce;
    size_t hashValue = hasher(ss.str());
    std::stringstream hexStream;
    hexStream << std::hex << hashValue;
    return hexStream.str();
}

// Fonction de minage par thread
void mineNonce(int index, std::string previousHash, std::string data, std::tm time, int difficulty, int offset, int step, int& validNonce)
{
    std::string target(difficulty, '0');
    int nonce = offset;
    int attempts = 0;  // Compteur de tentatives

    while (!found)
    {
        std::string hash = calculateHash(index, previousHash, data, time, nonce);
        if (hash.substr(0, difficulty) == target)
        {
            found = true;
            validNonce = nonce;  // Stocke le nonce valide
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Nonce found: " << nonce << " with hash: " << hash << std::endl;
            break;
        }

        nonce += step;  // Incr�ment par le "pas" donn� � chaque thread
        attempts++;

        // Afficher toutes les 1000 tentatives
        if (attempts % 1000 == 0)
        {
            std::lock_guard<std::mutex> lock(outputMutex);
            std::cout << "Thread " << std::this_thread::get_id() << " has tried " << attempts << " nonces." << std::endl;
        }
    }
}

class Block
{
protected:
    int index;
    std::string hash;
    std::string previousHash;
    std::tm time;
    std::string data;
    int nonce;

public:
    Block(int index, const std::string& data, Block* previousBlock = nullptr)
        : index(index), data(data), nonce(0)
    {
        if (previousBlock == nullptr)
        {
            previousHash = "0";
        }
        else
        {
            previousHash = previousBlock->getHash();
        }

        std::time_t t = std::time(nullptr);
        localtime_s(&time, &t);
        minageBlock(1);
    }

    void minageBlock(int difficulty)
    {
        int numThreads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        int validNonce = -1;  // Initialisation du nonce valide

        // Cr�ation de threads avec des offsets diff�rents
        for (int i = 0; i < numThreads; ++i)
        {
            threads.emplace_back(mineNonce, index, previousHash, data, time, difficulty, i, numThreads, std::ref(validNonce));
        }

        // Attendre que tous les threads finissent
        for (auto& th : threads)
        {
            if (th.joinable())
                th.join();
        }

        nonce = validNonce;  // On stocke le nonce trouv�
        hash = calculateHash(index, previousHash, data, time, nonce);
        std::cout << "Block mined with nonce: " << nonce << ", hash: " << hash << std::endl;
    }

    std::string printBlock() const
    {
        std::stringstream ss;
        ss << "Block numero : " << index << "\n";
        ss << "hash : " << hash << "\n";
        ss << "previous hash : " << previousHash << "\n";
        ss << "Date actuelle : "
            << (time.tm_year + 1900) << '-'
            << (time.tm_mon + 1) << '-'
            << time.tm_mday << " "
            << time.tm_hour << ':' << time.tm_min << ':' << time.tm_sec << "\n";
        ss << "donnee : " << data << "\n";
        return ss.str();
    }

    std::string getHash() const
    {
        return hash;
    }

    std::string getPreviousHash() const
    {
        return previousHash;
    }

    void setPreviousHash(const std::string& newPreviousHash)
    {
        previousHash = newPreviousHash;
    }

    void updateHash()
    {
        hash = calculateHash(index, previousHash, data, time, nonce);
    }

    void modifData(const std::string& newData)
    {
        data = newData;
        updateHash();
    }

    void corruptedBlock(const std::string& corruptedData)
    {
        data = corruptedData;
        std::time_t t = std::time(nullptr);
        localtime_s(&time, &t);
        updateHash();
    }

    void corruptedTime(std::tm& corruptedTime)
    {
        time = corruptedTime;
    }
};


class Blockchain
{
private:
    std::vector<Block> chain;

public:
    Blockchain()
    {
        chain.push_back(Block(0, "Genesis Block"));
    }

    void addBlock(const std::string& data)
    {
        int index = chain.size();
        Block* previousBlock = &chain.back();
        chain.push_back(Block(index, data, previousBlock));
    }

    // Corruption bloc 
    void corruptedBlock(int blockIndex, const std::string& corruptedData)
    {
        if (blockIndex < 0 || blockIndex >= chain.size())
        {
            std::cout << "Index du block invalide." << std::endl;
            return;
        }
        chain[blockIndex].corruptedBlock(corruptedData);
    }

    void corruptedBlockTime(int blockIndex, std::tm& corruptedTime)
    {
        if (blockIndex < 0 || blockIndex >= chain.size())
        {
            std::cout << "Index du block invalide." << std::endl;
            return;
        }
        chain[blockIndex].corruptedTime(corruptedTime);
        std::cout << "Horodatage du bloc " << blockIndex << " corrompu sans ajuster la chaine.\n";
    }

    void deleteBlock(int blockIndex)
    {
        if (blockIndex <= 0 || blockIndex >= chain.size())
        {
            std::cout << "Index du block invalide pour suppression." << std::endl;
            return;
        }

        chain.erase(chain.begin() + blockIndex);

        for (int i = blockIndex; i < chain.size(); ++i)
        {
            if (i == 0)
            {
                chain[i].setPreviousHash("0");
            }
            else
            {
                chain[i].setPreviousHash(chain[i - 1].getHash());
            }
            chain[i].updateHash();
        }

        std::cout << "Block " << blockIndex << " a ete supprime." << std::endl;
    }

    
    void vidange()
    {
        if (chain.size() > 1)
        {
            chain.erase(chain.begin() + 1, chain.end());
            std::cout << "Blockchain vidange sauf le block genesis \n" << std::endl;
        }

        else
        {
            std::cout << "Vidange impossible la Blockchain ne contient que le block genesis" << std::endl;
        }
    }

    void printBlockchainACote(const Blockchain& blockchainModifie, const std::string& messageOriginal, const std::string& messageModifie)
    {
        // Affichage message d'int�grit� en haut

        std::cout << std::left << std::setw(WIDTH) << messageOriginal
            << std::setw(WIDTH) << messageModifie << "\n";
        std::cout << std::string(WIDTH * 2, '=') << "\n";

        for (size_t i = 0; i < chain.size(); ++i)
        {
            std::string blockLeft = chain[i].printBlock();
            std::string blockRight = (i < blockchainModifie.chain.size()) ? blockchainModifie.chain[i].printBlock() : "";

            std::stringstream leftStream(blockLeft), rightStream(blockRight);
            std::string leftLine, rightLine;

            while (std::getline(leftStream, leftLine) || std::getline(rightStream, rightLine))
            {
                std::cout << std::left << std::setw(WIDTH) << leftLine
                    << std::setw(WIDTH) << rightLine << "\n";
            }

            std::cout << std::string(WIDTH * 2, '-') << "\n";
        }
    }

    // V�rifie si le hash d'un block est le m�me que le previous et stock un message en cons�quence
    std::string verifIntegrite()
    {
        bool estIntegre = true;
        std::stringstream message;

        for (size_t i = 1; i < chain.size(); ++i) // on commence � 1 � cause du genesis
        {
            if (chain[i].getPreviousHash() != chain[i - 1].getHash())
            {
                estIntegre = false;
                message << "Bloc " << i - 1 << " corrompu !";
            }
        }

        if (estIntegre)
        {
            message << "La blockchain est integre.";
        }
        else
        {
            message << "La blockchain n'est pas integre.";
        }

        return message.str(); 
    }

    Blockchain clone()
    {
        Blockchain newBlockchain;
        newBlockchain.chain = chain;
        return newBlockchain;
    }
};

int main()
{
    Blockchain myBlockchain;

    myBlockchain.addBlock("Ceci est une donnee");
    myBlockchain.addBlock("Les nuages dansaient lentement dans le ciel,\n peignant des formes mysterieuses au-dessus \n de la ville endormie.");
    myBlockchain.addBlock("Le parfum enivrant des roses emplissait l'air,\n transportant les souvenirs d'un ete lointain.");
    myBlockchain.addBlock("Les eclats de rire resonnaient a travers la foret,\n accompagnant le murmure apaisant du ruisseau voisin.");
    myBlockchain.addBlock("Ceci est une nouvelle donnee");

    Blockchain originalBlockchain = myBlockchain.clone();

    myBlockchain.deleteBlock(2);
    myBlockchain.corruptedBlock(3, "TENTATIVE CORRUPTION !!!!!!!!!!");

    std::cout << std::left << std::setw(WIDTH) << "Blockchain Originale"
        << std::setw(WIDTH) << "Blockchain Modifiee" << "\n";
    std::cout << std::string(WIDTH * 2, '=') << "\n";

    // V�rification integrite et recup les messages + affichage de l'ensemble
    std::string messageIntegriteOriginale = originalBlockchain.verifIntegrite();
    std::string messageIntegriteModifiee = myBlockchain.verifIntegrite();
    originalBlockchain.printBlockchainACote(myBlockchain, messageIntegriteOriginale, messageIntegriteModifiee);

    myBlockchain.vidange();
    myBlockchain.printBlockchainACote(myBlockchain, messageIntegriteOriginale, messageIntegriteModifiee);

    return 0;
}
