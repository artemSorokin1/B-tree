#include <iostream>
#include <cstring>
#include <cstdint>
#include <cctype>
#include <fstream>
#include <string>

const int T = 10;
const int T2 = 2 * T;

struct Pair {
    char key[257];
    uint64_t value;
};

class Node {
public:
    Pair keys[T];
    Node *children[T2 + 1];

    bool leaf = true;
    int keyNumbers = 0;

    Node () {
        for (int i = 0; i < T2 + 1; i++)
            children[i] = nullptr;
    }
};

bool operator<(const Pair &left, const char *right) {
    return strcmp(left.key, right) < 0;
}

bool operator<(const char *left, const Pair &right) {
    return strcmp(left, right.key) < 0;
}

bool operator==(const Pair &left, const char *right) {
    return strcmp(left.key, right) == 0;
}

bool operator==(const char *left, const Pair &right) {
    return strcmp(left, right.key) == 0;
}

bool operator<(const Pair& left, const Pair& right) {
    return strcmp(left.key, right.key) < 0;
}

class Btree {
public:
    Btree();
    ~Btree();

    void search(char *str) const;
    void insert(Pair &keyValue);
    void remove(char *str);

    void serialize(FILE* file);
    void deserialize(FILE* file);

private:
    Node *root;

    void searchNode(Node *node, char *str, Node *&res, int &pos) const;
    int searchInNode(Node *node, char *str);

    void insertInNode(Node *node, Pair &keyValue);

    void removeNode(Node *node, char *str);
    void removeFromNode(Node *node, int pos);
    void removeTree(Node *node);

    void mergeNodes(Node *parent, int pos);
    void splitChild(Node *parent, int pos);

    void balance(Node *node, int &pos);
    bool nodeToFile(Node *node, FILE *file);
    bool fileToTree(Node *node, FILE *file);

};

Btree::Btree() {
    root = new Node;
}

Btree::~Btree() {
    removeTree(root);
}

void Btree::removeTree(Node *node) {
    if (node == nullptr)
        return;
    if (!node -> leaf) { // поддерево
        for (int i = 0; i < T2 + 1; i++) {
            removeTree(node -> children[i]);
            node -> children[i] = nullptr;
        }
    }
    delete node;
}
void Btree::searchNode(Node *node, char *str, Node *&res, int &pos) const {
    int i = 0;
    res = nullptr;
    pos = - 1;
    char strLower[257];
    strcpy(strLower, str);
    for (int j = 0; strLower[j]!= '\0'; j++)
        strLower[j] = tolower(strLower[j]);

    while (i < node -> keyNumbers && strcmp(strLower, node -> keys[i].key) > 0)
        i++;

    if (i < node -> keyNumbers && strcmp(strLower, node -> keys[i].key) == 0) {
        res = node;
        pos = i;
        return;
    }
    if (node -> leaf) {
        res = nullptr;
        return;
    }
    searchNode(node -> children[i], str, res, pos);
}


int Btree::searchInNode(Node *node, char *str) {
    int left = -1;
    int right = node -> keyNumbers;
    while (right - left > 1) {
        int mediana = (right + left) / 2;
        if (std::tolower(node -> keys[mediana].key[0]) < std::tolower(str[0]))
            left = mediana;
        else
            right = mediana;
    }
    if (right < node -> keyNumbers && std::tolower(node -> keys[right].key[0]) == std::tolower(str[0]))
        return right;

    return -right -1;
}

void Btree::insertInNode(Node *node, Pair &keyValue) {
    int i = node -> keyNumbers - 1;

    while (i >= 0 && keyValue < node -> keys[i]) {
        node -> keys[i + 1] = node -> keys[i];
        i--;
    }
    node -> keys[i + 1] = keyValue;
    node -> keyNumbers++;
}

void Btree::removeFromNode(Node *node, int pos) {
    // сдвиг
    for (int i = pos; i < node -> keyNumbers - 1; i++)
        node -> keys[i] = node -> keys[i + 1];
    if (!node -> leaf) {
        for (int i = pos; i < node -> keyNumbers; i++) {
            node -> children[i] = node -> children[i + 1];
            node -> children[i + 1] = nullptr;
        }
        node -> keyNumbers--;
    }
}

void Btree::removeNode(Node *node, char *str) {
    int pos = -1;
    pos = searchInNode(node, str);
    Node *leftChild = nullptr;
    Node *rightChild = nullptr;
    // лист
    if (node -> leaf && pos >= 0) {
        removeFromNode(node, pos);
        return;
    }
    // не лист
    if (!node -> leaf && pos >= 0) {
        Node *leftChild = node -> children[pos];
        // налево до победного
        if (leftChild -> keyNumbers >= T) {
            while (!leftChild -> leaf)
                leftChild = leftChild -> children[leftChild -> keyNumbers];

            node -> keys[pos] = leftChild -> keys[leftChild -> keyNumbers -1];

            char *strNew = new char[strlen(leftChild -> keys[leftChild -> keyNumbers - 1].key) + 1];
            strcpy(strNew, leftChild -> keys[leftChild -> keyNumbers - 1].key);
            removeNode(leftChild, strNew);
            delete[] strNew;
            return;
        }

        // 1 справа
        Node *rightChild = node -> children[pos + 1];
        if (rightChild -> keyNumbers >= T) {
            while (!rightChild -> leaf)
                rightChild = rightChild -> children[0];

            node -> keys[pos] = rightChild -> keys[0];

            char *strNew = new char[strlen(rightChild -> keys[0].key) + 1];
            strcpy(strNew, rightChild -> keys[0].key);
            removeNode(node -> children[pos + 1], strNew);
            delete[] strNew;
            return;
        }
        mergeNodes(node, pos);
        removeNode(node -> children[pos], str);
        return;
    }
    // не нашли (левый сын)
    pos = -pos - 1;
    if (node -> children[pos] -> keyNumbers == T - 1)
        balance(node, pos);

    removeNode(node -> children[pos], str);
}

void Btree::mergeNodes(Node *parent, int pos) {
    Node *leftChild = parent -> children[pos];
    Node *rightChild = parent -> children[pos + 1];

    leftChild -> keys[leftChild -> keyNumbers++] = parent -> keys[pos];
    for (int i = 0; i < rightChild -> keyNumbers; i++)
        leftChild -> keys[leftChild -> keyNumbers++] = rightChild ->keys[i];
    if (!rightChild -> leaf) {
        for (int i = 0; i < rightChild -> keyNumbers; i++) {
            leftChild -> children[T + i] = rightChild -> children[i];
            rightChild -> children[i] = nullptr;
        }
    }
    rightChild -> keyNumbers = 0;
    delete rightChild;
    removeFromNode(parent, pos);
    parent -> children[pos] = leftChild;
}

void Btree::splitChild(Node *parent, int pos) {
    // в новый узел половина копии
    Node *child = parent -> children[pos];
    Node *newChild = new Node;
    newChild -> leaf = child -> leaf;
    newChild -> keyNumbers = T - 1;

    for (int i = 0; i < T - 1; i++)
        newChild -> keys[i] = child -> keys[T + i];
    if (!child -> leaf) {
        for (int i = 0; i < T; i++) {
            newChild -> children[i] = child -> children[T + i];
            child -> children[T + i] = nullptr;
        }
    }
    child -> keyNumbers = T - 1;

    for (int i = parent -> keyNumbers; i >= pos + 1; i--) {
        parent -> children[i + 1] = parent -> children[i];
        parent -> children[i] = nullptr;
    }
    parent -> children[pos + 1] = newChild;

    for (int i = parent -> keyNumbers - 1; i >= pos; i--)
        parent -> keys[i + 1] = parent -> keys[i];
    parent -> keys[pos] = child -> keys[T - 1];
    parent -> keyNumbers++;
}

void Btree::balance(Node *node, int& pos) {
    Node *child = node -> children[pos];
    // с левым братом, если у него можно стырить
    if (pos - 1 >= 0 && node -> children[pos - 1] -> keyNumbers >= T) {
        Node *leftBrother = node -> children[pos - 1];
        // сдвиг вправо
        for (int i = child -> keyNumbers - 1; i >= 0; i--)
            child -> keys[i + 1] = child -> keys[i];
        if (!child -> leaf) {
            for (int i = child -> keyNumbers; i >= 0; i--) {
                child -> children[i + 1] = child -> children[i];
                child -> children[i] = nullptr;
            }
        }
        // копии
        child -> keys[0] = node -> keys[pos - 1];
        child -> keyNumbers++;
        child -> children[0] = leftBrother -> children[leftBrother -> keyNumbers];
        leftBrother -> children[leftBrother -> keyNumbers] = nullptr;
        node -> keys[pos - 1] = leftBrother -> keys[leftBrother -> keyNumbers - 1];
        leftBrother -> keyNumbers--;

    } else if (pos + 1 <= node -> keyNumbers && node -> children[pos + 1] -> keyNumbers >= T) {
        // правый брат
        Node *rightBrother = node -> children[pos + 1];
        child -> keys[child -> keyNumbers] = node -> keys[pos];
        child -> children[child -> keyNumbers + 1] = rightBrother -> children[0];
        rightBrother -> children[0] = nullptr;
        child -> keyNumbers++;
        node -> keys[pos] = rightBrother -> keys[0];
        removeFromNode(rightBrother, 0);

    } else if (pos - 1 >= 0 && node -> children[pos - 1] -> keyNumbers == T - 1) {
        mergeNodes(node, pos - 1);
        pos--;

    } else if (pos + 1 <= node -> keyNumbers && node -> children[pos + 1] -> keyNumbers == T - 1)
        mergeNodes(node, pos);
}

bool Btree::nodeToFile(Node *node, FILE *file) {
    if (fwrite(&node -> leaf, sizeof(bool), 1, file) == 0)
        return false; // лист
    if (fwrite(&node -> keyNumbers, sizeof(int), 1, file) == 0)
        return false; // колво ключей
    for (int i = 0; i < node -> keyNumbers; i++) { // ключи записываем
        int len = strlen(node -> keys[i].key);
        if (fwrite(&len, sizeof(int), 1, file) == 0)
            return false;
        if (fwrite(node -> keys[i].key, sizeof(char), len, file) == 0)
            return false;

        if (fwrite(&node -> keys[i].value, sizeof(uint64_t), 1, file) == 0)
            return false;
    }
    if (!node -> leaf) {
        for (int i = 0; i < node -> keyNumbers + 1; i++)
            nodeToFile(node -> children[i], file);
    }
    return true;
}

bool Btree::fileToTree(Node *node, FILE *file) {
    if (fread(&node -> leaf, sizeof(bool), 1, file) == 0)
        return false; // из листа

    if (fread(&node -> keyNumbers, sizeof(int), 1, file) == 0)
        return false; // колво ключей

    // ключи из файла
    for (int i = 0; i < node -> keyNumbers; i++) {
        int len;
        if (fread(&len, sizeof(int), 1, file) == 0)
            return false;
        if (fread(node -> keys[i].key, sizeof(char), len, file) == 0)
            return false;
        node -> keys[i].key[len] = '\0';
        if (fread(&node -> keys[i].value, sizeof(uint64_t), 1, file) == 0)
            return false;
    }

    // лист -> удал детей
    if (node -> leaf) {
        for (int i = 0; i < T2 + 1; i++) {
            if (node -> children[i]!= nullptr) {
                removeTree(node -> children[i]);
                node -> children[i] = nullptr;
            }
        }
    } else { // рид
        for (int i = 0; i < node -> keyNumbers + 1; i++) {
            if (node -> children[i] == nullptr) {
                node -> children[i] = new Node;
            }
            fileToTree(node -> children[i], file);
        }
    }
    return true;
}


void Btree::search(char *str) const {
    Node *res;
    int pos;
    searchNode(root, str, res, pos);
    if (res == nullptr)
        std::cout << "NoSuchWord\n";
    else
        std::cout << "OK: " << res -> keys[pos].value << "\n";
}

void Btree::insert(Pair &keyValue) {
    Node *res;
    int pos;
    searchNode(root, keyValue.key, res, pos);
    if (res != nullptr) {
        std::cout << "Exist\n";
        return;
    }
    // корень переполнен
    if (root -> keyNumbers == T2 - 1) {
        Node *newRoot = new Node;
        newRoot -> leaf = false;
        newRoot -> children[0] = root;
        splitChild(newRoot, 0);
    }
    insertInNode(root, keyValue);
    std::cout << "OK\n";
}
void Btree::remove(char *str) {
    Node *res;
    int pos;
    searchNode(root, str, res, pos);
    if (res == nullptr)
        std::cout << "NoSuchWord\n";
    else {
        removeNode(root, str);
        std::cout << "OK\n";
        // корень пустой
        if (!root -> leaf &&root -> keyNumbers == 0) {
            Node *tmp = root -> children[0];
            delete root;
            root = tmp;
        }
    }
}

void Btree::serialize(FILE* file){
    nodeToFile(root, file);
}

void Btree::deserialize(FILE* file) {
    removeTree(root);
    fileToTree(root, file);
}

int main() {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);
    Btree tree;
    char input[257];
    while (std::cin >> input) {
        if (strcmp(input, "+") == 0) {
            Pair keyValue;
            std::cin >> keyValue.key >> keyValue.value;
            tree.insert(keyValue);
        } else if (strcmp(input, "-") == 0) {
            std::cin >> input;
            tree.remove(input);
        } else if (strcmp(input, "!") == 0) {
            std::cin >> input;
            if (strcmp(input, "Save") == 0) {
                std::cin >> input;
                FILE *file = fopen(input, "wb");
                if (!file)
                    std::cout << "ERROR \n";
                else {
                    tree.serialize(file);
                    fclose(file);
                }
            } else if (strcmp(input, "Load") == 0) {
                std::cin >> input;
                FILE *file = fopen(input, "rb");
                if (!file)
                    std::cout << "ERROR \n";
                else {
                    tree.deserialize(file);
                    fclose(file);
                }
            }
        } else
            tree.search(input);
    }
}