#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

using namespace std;

/*
 * Binary-Safe RLE Compression Algorithm
 *
 * Format: Uses escape byte (0xFF) to mark compressed runs
 * - For runs of 4+ identical bytes: [0xFF][count][byte]
 * - For literal bytes: output as-is (escape 0xFF as 0xFF 0x00)
 * - Count is stored as 1 byte (max run = 255)
 */

const uint8_t ESCAPE_BYTE = 0xFF;
const int MIN_RUN_LENGTH = 4; // Minimum run length to compress

// Binary-safe RLE Compression
vector<uint8_t> rleCompressBinary(const vector<uint8_t> &input)
{
    if (input.empty())
        return {};

    vector<uint8_t> compressed;
    size_t i = 0;

    while (i < input.size())
    {
        uint8_t currentByte = input[i];
        size_t runLength = 1;

        // Count consecutive identical bytes
        while (i + runLength < input.size() &&
               input[i + runLength] == currentByte &&
               runLength < 255)
        {
            runLength++;
        }

        if (runLength >= MIN_RUN_LENGTH)
        {
            // Encode as: ESCAPE_BYTE + count + byte
            compressed.push_back(ESCAPE_BYTE);
            compressed.push_back(static_cast<uint8_t>(runLength));
            compressed.push_back(currentByte);
            i += runLength;
        }
        else
        {
            // Output literal bytes
            for (size_t j = 0; j < runLength; j++)
            {
                if (currentByte == ESCAPE_BYTE)
                {
                    // Escape the escape byte: 0xFF -> 0xFF 0x00
                    compressed.push_back(ESCAPE_BYTE);
                    compressed.push_back(0x00);
                }
                else
                {
                    compressed.push_back(currentByte);
                }
            }
            i += runLength;
        }
    }

    return compressed;
}

// Binary-safe RLE Decompression
vector<uint8_t> rleDecompressBinary(const vector<uint8_t> &compressed)
{
    if (compressed.empty())
        return {};

    vector<uint8_t> decompressed;
    size_t i = 0;

    while (i < compressed.size())
    {
        if (compressed[i] == ESCAPE_BYTE)
        {
            if (i + 1 >= compressed.size())
            {
                cerr << "Error: Unexpected end of compressed data" << endl;
                break;
            }

            uint8_t count = compressed[i + 1];

            if (count == 0x00)
            {
                // Escaped escape byte: 0xFF 0x00 -> 0xFF
                decompressed.push_back(ESCAPE_BYTE);
                i += 2;
            }
            else
            {
                // Run-length encoded: ESCAPE + count + byte
                if (i + 2 >= compressed.size())
                {
                    cerr << "Error: Unexpected end of compressed data" << endl;
                    break;
                }
                uint8_t byte = compressed[i + 2];
                for (uint8_t j = 0; j < count; j++)
                {
                    decompressed.push_back(byte);
                }
                i += 3;
            }
        }
        else
        {
            // Literal byte
            decompressed.push_back(compressed[i]);
            i++;
        }
    }

    return decompressed;
}

// Read binary file into vector
vector<uint8_t> readBinaryFile(const string &filename)
{
    ifstream file(filename, ios::binary | ios::ate);
    if (!file)
    {
        cerr << "Error: Cannot open file: " << filename << endl;
        return {};
    }

    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char *>(buffer.data()), size))
    {
        cerr << "Error: Failed to read file: " << filename << endl;
        return {};
    }

    return buffer;
}

// Write vector to binary file
bool writeBinaryFile(const string &filename, const vector<uint8_t> &data)
{
    ofstream file(filename, ios::binary);
    if (!file)
    {
        cerr << "Error: Cannot create file: " << filename << endl;
        return false;
    }

    file.write(reinterpret_cast<const char *>(data.data()), data.size());
    return file.good();
}

// Compress a file
bool compressFile(const string &inputFile, const string &outputFile)
{
    cout << "Reading file: " << inputFile << endl;

    vector<uint8_t> input = readBinaryFile(inputFile);
    if (input.empty())
    {
        cerr << "Error: Input file is empty or could not be read" << endl;
        return false;
    }

    cout << "Compressing..." << endl;
    vector<uint8_t> compressed = rleCompressBinary(input);

    if (!writeBinaryFile(outputFile, compressed))
    {
        return false;
    }

    double ratio = (compressed.size() * 100.0) / input.size();

    cout << "\n=== Compression Complete ===" << endl;
    cout << "Original size:   " << input.size() << " bytes" << endl;
    cout << "Compressed size: " << compressed.size() << " bytes" << endl;
    cout << "Compression ratio: " << ratio << "%" << endl;

    if (compressed.size() < input.size())
    {
        cout << "Space saved: " << (input.size() - compressed.size()) << " bytes" << endl;
    }
    else
    {
        cout << "Note: File did not compress well (random/already compressed data)" << endl;
    }

    return true;
}

// Decompress a file
bool decompressFile(const string &inputFile, const string &outputFile)
{
    cout << "Reading compressed file: " << inputFile << endl;

    vector<uint8_t> compressed = readBinaryFile(inputFile);
    if (compressed.empty())
    {
        cerr << "Error: Compressed file is empty or could not be read" << endl;
        return false;
    }

    cout << "Decompressing..." << endl;
    vector<uint8_t> decompressed = rleDecompressBinary(compressed);

    if (!writeBinaryFile(outputFile, decompressed))
    {
        return false;
    }

    cout << "\n=== Decompression Complete ===" << endl;
    cout << "Compressed size:   " << compressed.size() << " bytes" << endl;
    cout << "Decompressed size: " << decompressed.size() << " bytes" << endl;

    return true;
}

// Text string compression (for demo purposes)
string rleCompressText(const string &input)
{
    vector<uint8_t> inputVec(input.begin(), input.end());
    vector<uint8_t> compressed = rleCompressBinary(inputVec);
    return string(compressed.begin(), compressed.end());
}

string rleDecompressText(const string &compressed)
{
    vector<uint8_t> compressedVec(compressed.begin(), compressed.end());
    vector<uint8_t> decompressed = rleDecompressBinary(compressedVec);
    return string(decompressed.begin(), decompressed.end());
}

int main()
{
    int choice;
    string input, output;

    cout << "========================================" << endl;
    cout << "   Binary-Safe RLE Compression Tool    " << endl;
    cout << "========================================" << endl;
    cout << "1. Compress a file" << endl;
    cout << "2. Decompress a file" << endl;
    cout << "3. Compress a string (demo)" << endl;
    cout << "4. Decompress a string (demo)" << endl;
    cout << "5. Exit" << endl;
    cout << "========================================" << endl;

    while (true)
    {
        cout << "\nEnter choice (1-5): ";
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
        case 1:
        {
            cout << "Enter input file path: ";
            getline(cin, input);
            cout << "Enter output file path: ";
            getline(cin, output);
            compressFile(input, output);
            break;
        }
        case 2:
        {
            cout << "Enter compressed file path: ";
            getline(cin, input);
            cout << "Enter output file path: ";
            getline(cin, output);
            decompressFile(input, output);
            break;
        }
        case 3:
        {
            cout << "Enter string to compress: ";
            getline(cin, input);
            string compressed = rleCompressText(input);
            cout << "Original length: " << input.length() << endl;
            cout << "Compressed length: " << compressed.length() << endl;
            break;
        }
        case 4:
        {
            cout << "Enter string to decompress: ";
            getline(cin, input);
            string decompressed = rleDecompressText(input);
            cout << "Decompressed: " << decompressed << endl;
            break;
        }
        case 5:
        {
            cout << "Goodbye!" << endl;
            return 0;
        }
        default:
            cout << "Invalid choice. Please try again." << endl;
        }
    }

    return 0;
}
