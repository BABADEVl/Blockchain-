#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>

// Pour l'affichage
const int WIDTH = 60;

// Calcule du hash avec l'index du block, le previous hash, la data, l'horodatage
std::string calculateHash(int index, const std::string& previousHash, const std::string& data, const std::tm& time)
{
    std::hash<std::string> hasher;
    std::stringstream ss;
    ss << index << previousHash << data << std::put_time(&time, "%Y-%m-%d %H:%M:%S");
    size_t hashValue = hasher(ss.str());
    std::stringstream hexStream;
    hexStream << std::hex << hashValue;
    return hexStream.str();
}

class Block
{
protected:
    int index;
    std::string hash;
    std::string previousHash;
    std::tm time;
    std::string data;

public:
    Block(int index, const std::string& data, Block* previousBlock = nullptr) : index(index), data(data)
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
        hash = calculateHash(index, previousHash, data, time);
    }

    std::string printBlock() const
    {
        std::stringstream ss;
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
        hash = calculateHash(index, previousHash, data, time);
    }

    void modifData(const std::string& newData)
    {
        data = newData;
        updateHash();
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

    void modifBlock(int blockIndex, const std::string& newData)
    {
        if (blockIndex < 0 || blockIndex >= chain.size())
        {
            std::cout << "Index du block invalide." << std::endl;
            return;
        }

        // Modifier bloc
        chain[blockIndex].modifData(newData);

        // Recalculer les hashs d'après
        for (int i = blockIndex + 1; i < chain.size(); ++i)
        {
            chain[i].setPreviousHash(chain[i - 1].getHash());
            chain[i].updateHash();
        }
    }

    void printBlockchainACote(const Blockchain& blockchainModife)
    {
        for (size_t i = 0; i < chain.size(); ++i)
        {
            std::string blockLeft = chain[i].printBlock();
            std::string blockRight = (i < blockchainModife.chain.size()) ? blockchainModife.chain[i].printBlock() : "";

            std::stringstream leftStream(blockLeft), rightStream(blockRight);
            std::string leftLine, rightLine;

            //Affiche les chaînes ligne par ligne
            while (std::getline(leftStream, leftLine) || std::getline(rightStream, rightLine))
            {
                std::cout << std::left << std::setw(WIDTH) << leftLine
                    << std::setw(WIDTH) << rightLine << "\n";
            }
            // Séparateur pour esthetique
            std::cout << std::string(WIDTH * 2, '-') << "\n";
        }
    }
    // Copie tous les bloc pour les comparés
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

    // Cloner la blockchain avant modif
    Blockchain originalBlockchain = myBlockchain.clone();

    // Modif d'un block
    myBlockchain.modifBlock(2, "TENTATIVE CORRUPTION !!!!!!!!!!");

    std::cout << std::left << std::setw(WIDTH) << "Blockchain Originale"
        << std::setw(WIDTH) << "Blockchain Modifie" << "\n";
    std::cout << std::string(WIDTH * 2, '=') << "\n";

    // Afficher Blockachain
    originalBlockchain.printBlockchainACote(myBlockchain);

    return 0;
}
