#include <iostream>
#include <queue>
#include <unordered_map>
#include <vector>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <iomanip>
#include <string>
using namespace std;

// Huffman Tree Node
struct Node
{
    unsigned char ch;
    int freq;
    Node *left;
    Node *right;
    Node(unsigned char c, int f, Node *l = nullptr, Node *r = nullptr)
        : ch(c), freq(f), left(l), right(r) {}
};

// Comparator for priority queue
struct Compare
{
    bool operator()(Node *a, Node *b) { return a->freq > b->freq; }
};

// Free Huffman tree recursively
void freeTree(Node *node)
{
    if (!node)
        return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

// Build Huffman Tree from frequency map
Node *buildHuffmanTree(const unordered_map<unsigned char, int> &freqMap)
{
    priority_queue<Node *, vector<Node *>, Compare> pq;
    for (auto &[c, f] : freqMap)
        pq.push(new Node(c, f));
    if (pq.size() == 1)
    {
        Node *onlyNode = pq.top();
        return new Node('\0', onlyNode->freq, onlyNode, nullptr);
    }
    while (pq.size() > 1)
    {
        Node *left = pq.top();
        pq.pop();
        Node *right = pq.top();
        pq.pop();
        pq.push(new Node('\0', left->freq + right->freq, left, right));
    }
    return pq.top();
}

// Build Huffman codes recursively
void buildCodes(Node *node, const string &prefix, unordered_map<unsigned char, string> &codes)
{
    if (!node)
        return;
    if (!node->left && !node->right)
    {
        codes[node->ch] = prefix.empty() ? "1" : prefix;
    }
    else
    {
        buildCodes(node->left, prefix + "1", codes);
        buildCodes(node->right, prefix + "0", codes);
    }
}

// Write bits to file using buffer
void writeBits(ofstream &out, const string &bits, uint8_t &buffer, int &count)
{
    for (char b : bits)
    {
        buffer <<= 1;
        if (b == '1')
            buffer |= 1;
        count++;
        if (count == 8)
        {
            out.put(buffer);
            buffer = 0;
            count = 0;
        }
    }
}

// Flush remaining bits
void flushBits(ofstream &out, uint8_t &buffer, int &count)
{
    if (count > 0)
    {
        buffer <<= (8 - count);
        out.put(buffer);
        buffer = 0;
        count = 0;
    }
}

// Save canonical Huffman table
void saveCanonicalTable(ofstream &out, const unordered_map<unsigned char, string> &codes)
{
    uint16_t tableSize = codes.size();
    out.write(reinterpret_cast<char *>(&tableSize), sizeof(tableSize));

    vector<pair<unsigned char, uint8_t>> table;
    for (auto &[c, code] : codes)
        table.push_back({c, static_cast<uint8_t>(code.size())});
    sort(table.begin(), table.end(), [](auto &a, auto &b)
         { return a.second == b.second ? a.first < b.first : a.second < b.second; });

    for (auto &[c, len] : table)
    {
        out.put(c);
        out.put(len);
    }
}

// Load canonical Huffman table and reconstruct codes
unordered_map<unsigned char, string> loadCanonicalTable(ifstream &in)
{
    unordered_map<unsigned char, string> codes;
    uint16_t tableSize;
    in.read(reinterpret_cast<char *>(&tableSize), sizeof(tableSize));

    vector<pair<unsigned char, uint8_t>> table(tableSize);
    for (int i = 0; i < tableSize; i++)
    {
        table[i] = {static_cast<unsigned char>(in.get()), static_cast<uint8_t>(in.get())};
    }
    sort(table.begin(), table.end(), [](auto &a, auto &b)
         { return a.second == b.second ? a.first < b.first : a.second < b.second; });

    uint32_t code = 0;
    uint8_t prevLen = 0;
    for (auto &[c, len] : table)
    {
        code <<= (len - prevLen);
        string codeStr;
        for (int i = len - 1; i >= 0; i--)
            codeStr += ((code >> i) & 1) ? '1' : '0';
        codes[c] = codeStr;
        code++;
        prevLen = len;
    }
    return codes;
}

// Build canonical codes from code lengths
unordered_map<unsigned char, string> makeCanonicalCodes(const unordered_map<unsigned char, string> &codes)
{
    vector<pair<unsigned char, uint8_t>> table;
    for (auto &[c, code] : codes)
        table.push_back({c, static_cast<uint8_t>(code.size())});

    sort(table.begin(), table.end(), [](auto &a, auto &b)
         { return a.second == b.second ? a.first < b.first : a.second < b.second; });

    unordered_map<unsigned char, string> canonical;
    uint32_t codeVal = 0;
    uint8_t prevLen = 0;
    for (auto &[c, len] : table)
    {
        codeVal <<= (len - prevLen);
        string codeStr;
        for (int i = len - 1; i >= 0; i--)
            codeStr += ((codeVal >> i) & 1) ? '1' : '0';
        canonical[c] = codeStr;
        codeVal++;
        prevLen = len;
    }

    return canonical;
}

// Get file size without moving the stream on return
uint64_t getFileSize(ifstream &in)
{
    auto current = in.tellg();
    in.seekg(0, ios::end);
    auto endPos = in.tellg();
    in.seekg(current, ios::beg);
    if (endPos == static_cast<streampos>(-1))
        return 0;
    return static_cast<uint64_t>(endPos);
}

// Compress file in chunks
void compressFile(const string &inputFile, const string &outputFile, size_t blockSize = 1 << 20)
{
    ifstream in(inputFile, ios::binary);
    ofstream out(outputFile, ios::binary);
    if (!in || !out)
    {
        cerr << "Error opening files!\n";
        return;
    }

    uint64_t totalBytes = getFileSize(in);
    uint64_t processed = 0;

    while (!in.eof())
    {
        vector<unsigned char> block(blockSize);
        in.read(reinterpret_cast<char *>(block.data()), blockSize);
        size_t readBytes = in.gcount();
        block.resize(readBytes);
        if (readBytes == 0)
            break;

        unordered_map<unsigned char, int> freq;
        for (unsigned char c : block)
            freq[c]++;

        Node *tree = buildHuffmanTree(freq);
        unordered_map<unsigned char, string> codes;
        buildCodes(tree, "", codes);
        unordered_map<unsigned char, string> canonicalCodes = makeCanonicalCodes(codes);

        uint8_t buffer = 0;
        int count = 0;
        int bitLength = 0;
        for (unsigned char c : block)
            bitLength += canonicalCodes[c].size();
        uint32_t bits = bitLength;
        out.write(reinterpret_cast<char *>(&bits), sizeof(bits));

        saveCanonicalTable(out, canonicalCodes);

        for (unsigned char c : block)
            writeBits(out, canonicalCodes[c], buffer, count);
        flushBits(out, buffer, count);

        freeTree(tree);

        processed += readBytes;
        if (totalBytes > 0)
        {
            double pct = (static_cast<double>(processed) * 100.0) / static_cast<double>(totalBytes);
            if (pct > 100.0)
                pct = 100.0;
            cout << "\rCompressing: " << fixed << setprecision(1) << pct << "%" << flush;
        }
    }

    in.close();
    out.close();
    if (totalBytes > 0)
        cout << "\rCompressing: 100.0%\n";
    cout << "Compression complete!\n";
}

// Decode block from file
vector<unsigned char> decodeBlock(ifstream &in, uint32_t bitLength)
{
    unordered_map<unsigned char, string> codes = loadCanonicalTable(in);

    Node *root = new Node('\0', 0);
    for (auto &[c, code] : codes)
    {
        Node *node = root;
        for (char b : code)
        {
            if (b == '1')
            {
                if (!node->left)
                    node->left = new Node('\0', 0);
                node = node->left;
            }
            else
            {
                if (!node->right)
                    node->right = new Node('\0', 0);
                node = node->right;
            }
        }
        node->ch = c;
    }

    vector<unsigned char> decoded;
    Node *node = root;
    int bitsRead = 0;
    while (bitsRead < bitLength)
    {
        int val = in.get();
        if (val == EOF)
            break;
        uint8_t byte = static_cast<uint8_t>(val);
        for (int i = 7; i >= 0 && bitsRead < bitLength; i--)
        {
            bool bit = (byte >> i) & 1;
            node = bit ? node->left : node->right;
            if (!node->left && !node->right)
            {
                decoded.push_back(node->ch);
                node = root;
            }
            bitsRead++;
        }
    }

    freeTree(root);
    return decoded;
}

// Decompress file in chunks
void decompressFile(const string &inputFile, const string &outputFile)
{
    ifstream in(inputFile, ios::binary);
    ofstream out(outputFile, ios::binary);
    if (!in || !out)
    {
        cerr << "Error opening files!\n";
        return;
    }

    uint64_t totalBytes = getFileSize(in);
    uint64_t processed = 0;
    in.clear();
    in.seekg(0, ios::beg);

    while (!in.eof())
    {
        streampos blockStart = in.tellg();
        uint32_t bitLength;
        in.read(reinterpret_cast<char *>(&bitLength), sizeof(bitLength));
        if (in.eof())
            break;
        vector<unsigned char> block = decodeBlock(in, bitLength);
        out.write(reinterpret_cast<char *>(block.data()), block.size());

        streampos afterBlock = in.tellg();
        if (blockStart != static_cast<streampos>(-1) && afterBlock != static_cast<streampos>(-1))
        {
            processed += static_cast<uint64_t>(afterBlock - blockStart);
            if (totalBytes > 0)
            {
                double pct = (static_cast<double>(processed) * 100.0) / static_cast<double>(totalBytes);
                if (pct > 100.0)
                    pct = 100.0;
                cout << "\rDecompressing: " << fixed << setprecision(1) << pct << "%" << flush;
            }
        }
    }

    in.close();
    out.close();
    if (totalBytes > 0)
        cout << "\rDecompressing: 100.0%\n";
    cout << "Decompression complete!\n";
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        cerr << "Usage: " << argv[0] << " c <input> <compressed>\n"
             << "   or: " << argv[0] << " d <compressed> <output>\n";
        return 1;
    }

    string mode = argv[1];
    string first = argv[2];
    string second = argv[3];

    if (mode == "c")
    {
        compressFile(first, second);
    }
    else if (mode == "d")
    {
        decompressFile(first, second);
    }
    else
    {
        cerr << "Unknown mode: " << mode << " (use 'c' for compress, 'd' for decompress)\n";
        return 1;
    }

    return 0;
}
