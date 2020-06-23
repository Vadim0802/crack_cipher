#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <queue>
#include <condition_variable>
#include <atomic>
#include <memory>

class keyStruct
{
private:
    int             _lengthKey;
    std::string     _status;
public:
    keyStruct(int length, std::string status)
    {
        this->_lengthKey = length;
        this->_status = status;
    }
    std::string getStatus() { return _status; };
    void setStatus(std::string status) { this->_status = status; };
    int getLength() { return _lengthKey; };
};

std::vector<std::shared_ptr<keyStruct>>        _queueKey;
std::mutex                                     _mutex;
std::condition_variable                        _condition;
std::atomic<bool>                              _searchKey = true;
class Crack
{
protected:
    std::ifstream                   _readFile;
    std::vector<std::string>        _words;
    std::vector<std::string>        _encryptLine;
    std::string                     _pathToEncryptFile;
    std::string                     _pathToWords;
    std::vector<int>                _key;
    std::vector<std::thread>        _threads;


    void swap(int* a, int i, int j);
    bool NextSet(int* a, int n);
    void readEncryptFile(std::string& _pathToEncryptFile);
    void readWords(std::string& _pathToWords);
    bool isTrueKey(int* a, int size);
    void iteratingKey(int size);
    void initThreads();
    int searchNotEmptyLine();
public:
    Crack(std::string& pathWords, std::string pathFile);
};

Crack::Crack(std::string& pathWords, std::string pathFile)
{
    this->_pathToWords = pathWords;
    this->_pathToEncryptFile = pathFile;
    readEncryptFile(pathFile);
    readWords(pathWords);
    int maxLengthKey = searchNotEmptyLine();
    for (int i = 2; i < maxLengthKey; i++)
    {
        if (maxLengthKey % i == 0)
        {
            keyStruct task(i, "inQueue");
            _queueKey.push_back(std::make_shared<keyStruct>(std::move(task)));
        }
    }
    initThreads();
    std::unique_lock<std::mutex> lock(_mutex);
    _condition.wait(lock);
    if (_key.empty())
        std::cout << "Не удалось расшифровать файл! Попробуйте использовать другой словарь." << std::endl;
    else
    {
        std::cout << "Файл расшифрован!" << std::endl;
        std::string pathSave;
        std::cout << "Введите путь для сохранения файла: "; std::cin >> pathSave;
        std::ofstream inputFile(pathSave);
        inputFile << "Ключ: ";
        for (int i = 0; i < _key.size(); i++)
            inputFile << _key[i]; inputFile << " ";
        inputFile << std::endl;

        for (int i = 0; i < _encryptLine.size(); i++)
        {
            std::string _tmpCorrectElem;
            for (int j = 0; j < _encryptLine[i].size() / _key.size(); j++)
            {
                for (int x = 0; x < _key.size(); x++)
                {
                    int _keyElem = j * _key.size();
                    int _position;
                    for (int y = 0; y < _key.size(); y++)
                    {
                        if (x + 1 == _key[y])
                            _position = y;
                    }
                    _tmpCorrectElem.push_back(_encryptLine[i][_keyElem + _position]);
                }
            }
            inputFile << _tmpCorrectElem << std::endl;
        }
        inputFile.close();
    }
}

void Crack::swap(int* a, int i, int j)
{

    int s = a[i];
    a[i] = a[j];
    a[j] = s;
}

bool Crack::NextSet(int* a, int n)
{
    int j = n - 2;
    while (j != -1 && a[j] >= a[j + 1]) j--;
    if (j == -1)
        return false;
    int k = n - 1;
    while (a[j] >= a[k]) k--;
    swap(a, j, k);
    int l = j + 1, r = n - 1;
    while (l < r)
        swap(a, l++, r--);
    return true;
}

void Crack::readEncryptFile(std::string& _pathToEncryptFile)
{
    _readFile.open(_pathToEncryptFile);
    if (_readFile.is_open())
    {
        while (!_readFile.eof())
        {
            std::string tmp;
            std::getline(_readFile, tmp);
            _encryptLine.push_back(tmp);
        }
        _readFile.close();
        if (_encryptLine.empty())
        {
            std::cout << "Файл с шифротекстом пустой!" << std::endl;
            exit(0);
        }
    }
    else
        std::cout << "Не удалось открыть файл!" << std::endl;
}

void Crack::readWords(std::string& _pathToWords)
{
    _readFile.open(_pathToWords);
    if (_readFile.is_open())
    {
        while (!_readFile.eof())
        {
            std::string tmp;
            std::getline(_readFile, tmp);
            _words.push_back(tmp);
        }
        _readFile.close();
        if (_encryptLine.empty())
        {
            std::cout << "Файл словаря пустой!" << std::endl;
            exit(0);
        }
    }
    else
        std::cout << "Не удалось открыть файл!" << std::endl;
}

bool Crack::isTrueKey(int* a, int size)
{
    std::vector<std::string> _tmpDecrypt;
    for (int i = 0; i < _encryptLine.size(); i++)
    {
        std::string _tmpCorrectElem;
        for (int j = 0; j < _encryptLine[i].size() / size; j++)
        {
            for (int x = 0; x < size; x++)
            {
                int _keyElem = j * size;
                int _position;
                for (int y = 0; y < size; y++)
                {
                    if (x + 1 == a[y])
                        _position = y;
                }
                _tmpCorrectElem.push_back(_encryptLine[i][_keyElem + _position]);
            }
        }
        _tmpDecrypt.push_back(_tmpCorrectElem);
    }
    int counterCorrectWords = 0;
    int counterNotCorrectWords = 0;

    for (int i = 0; i < _tmpDecrypt.size(); i++)
    {
        std::string _tmp;
        bool endWord = true;
        for (int j = 0; j < _tmpDecrypt[i].size(); j++)
        {
            if (_tmpDecrypt[i][j] == ' ' || (_tmpDecrypt[i][j] >= 33 && _tmpDecrypt[i][j] <= 46))
            {
                if (endWord)
                    continue;
                else
                {
                    bool isCorrectWord = false;
                    for (int x = 0; x < _words.size(); x++)
                    {
                        if (_tmp == _words[x])
                            isCorrectWord = true;
                    }
                    if (isCorrectWord)
                        counterCorrectWords++;
                    else
                        counterNotCorrectWords++;
                    _tmp.clear();
                    endWord = true;
                }
            }
            else
            {
                if (endWord)
                    endWord = false;
                _tmp.push_back(_tmpDecrypt[i][j]);
            }
        }
    }
    if (counterCorrectWords > counterNotCorrectWords&& counterNotCorrectWords == 0)
        return true;
    return false;
}

void Crack::iteratingKey(int size)
{
    bool searchTrueKey = false;
    int* arr = new int[size];

    for (int i = 0; i < size; i++)
        arr[i] = i + 1;

    if (isTrueKey(arr, size))
        searchTrueKey = true;
    else
    {
        while (NextSet(arr, size))
        {
            if (isTrueKey(arr, size))
            {
                searchTrueKey = true;
                break;
            }
        }
    }

    if (searchTrueKey)
    {
        if (_searchKey)
        {
            _mutex.lock();
            _searchKey = false;
            for (int i = 0; i < size; i++)
                _key.push_back(arr[i]);
            _mutex.unlock();
            _condition.notify_one();
        }
       
    }
}

void Crack::initThreads()
{
    for (int i = 0; i < std::thread::hardware_concurrency() - 1; i++)
    {
        _threads.emplace_back([&] {
            while (true)
            {
                for (int i = 0; i < _queueKey.size(); i++)
                {
                    auto task = _queueKey[i];
                    if (task->getStatus() == "inQueue")
                    {
                        _mutex.lock();
                        task->setStatus("Check");
                        _mutex.unlock();
                        iteratingKey(task->getLength());
                        task->setStatus("Checked");
                    }

                    bool _threadEndWork = true;
                    for (auto& iter : _queueKey)
                    {
                        if (iter->getStatus() != "Checked")
                            _threadEndWork = false;
                    }
                    if (!_searchKey)
                        break;

                    if (!_threadEndWork)
                        continue;

                    if (_threadEndWork && _searchKey)
                    {
                        _condition.notify_one();
                        break;
                    }
                }

            }
        });
        _threads.back().detach();
    }
}

int Crack::searchNotEmptyLine()
{
    for (auto& t : _encryptLine)
        if (!t.empty())
            return t.size();
}

int main()
{
    setlocale(LC_ALL, "RU");
    std::string pathFile;
    std::string pathWords;
    std::cout << "Введите путь к зашифрованному файлу: "; std::cin >> pathFile;
    std::cout << "Введите путь к словарю: "; std::cin >> pathWords;
    Crack decrypt(pathWords, pathFile);
}