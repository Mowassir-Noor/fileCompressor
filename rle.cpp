#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

using namespace std;

// RLE Compression: Encodes consecutive repeated characters as count + character
string rleCompress(const string &input)
{
    if (input.empty())
        return "";

    string compressed;
    int count = 1;

    for (size_t i = 1; i <= input.length(); i++)
    {
        if (i < input.length() && input[i] == input[i - 1])
        {
            count++;
        }
        else
        {
            // Append count and character
            compressed += to_string(count) + input[i - 1];
            count = 1;
        }
    }

    return compressed;
}

// RLE Decompression: Decodes count + character back to original string
string rleDecompress(const string &compressed)
{
    if (compressed.empty())
        return "";

    string decompressed;
    string countStr;

    for (size_t i = 0; i < compressed.length(); i++)
    {
        if (isdigit(compressed[i]))
        {
            countStr += compressed[i];
        }
        else
        {
            int count = stoi(countStr);
            decompressed += string(count, compressed[i]);
            countStr.clear();
        }
    }

    return decompressed;
}

// Compress a file and save to output file
bool compressFile(const string &inputFile, const string &outputFile)
{
    ifstream inFile(inputFile, ios::binary);
    if (!inFile)
    {
        cerr << "Error: Cannot open input file: " << inputFile << endl;
        return false;
    }

    // Read entire file content
    stringstream buffer;
    buffer << inFile.rdbuf();
    string content = buffer.str();
    inFile.close();

    // Compress the content
    string compressed = rleCompress(content);

    // Write compressed data to output file
    ofstream outFile(outputFile, ios::binary);
    if (!outFile)
    {
        cerr << "Error: Cannot open output file: " << outputFile << endl;
        return false;
    }

    outFile << compressed;
    outFile.close();

    cout << "Compression complete!" << endl;
    cout << "Original size: " << content.size() << " bytes" << endl;
    cout << "Compressed size: " << compressed.size() << " bytes" << endl;
    cout << "Compression ratio: " << (compressed.size() * 100.0 / content.size()) << "%" << endl;

    return true;
}

// Decompress a file and save to output file
bool decompressFile(const string &inputFile, const string &outputFile)
{
    ifstream inFile(inputFile, ios::binary);
    if (!inFile)
    {
        cerr << "Error: Cannot open input file: " << inputFile << endl;
        return false;
    }

    // Read entire file content
    stringstream buffer;
    buffer << inFile.rdbuf();
    string content = buffer.str();
    inFile.close();

    // Decompress the content
    string decompressed = rleDecompress(content);

    // Write decompressed data to output file
    ofstream outFile(outputFile, ios::binary);
    if (!outFile)
    {
        cerr << "Error: Cannot open output file: " << outputFile << endl;
        return false;
    }

    outFile << decompressed;
    outFile.close();

    cout << "Decompression complete!" << endl;
    cout << "Compressed size: " << content.size() << " bytes" << endl;
    cout << "Decompressed size: " << decompressed.size() << " bytes" << endl;

    return true;
}

int main()
{
    int choice;
    string input, output;

    cout << "===== RLE Compression Tool =====" << endl;
    cout << "1. Compress a string" << endl;
    cout << "2. Decompress a string" << endl;
    cout << "3. Compress a file" << endl;
    cout << "4. Decompress a file" << endl;
    cout << "5. Exit" << endl;
    cout << "================================" << endl;

    while (true)
    {
        cout << "\nEnter choice (1-5): ";
        cin >> choice;
        cin.ignore(); // Clear newline from buffer

        switch (choice)
        {
        case 1:
        {
            cout << "Enter string to compress: ";
            getline(cin, input);
            string compressed = rleCompress(input);
            cout << "Compressed: " << compressed << endl;
            cout << "Original length: " << input.length() << endl;
            cout << "Compressed length: " << compressed.length() << endl;
            break;
        }
        case 2:
        {
            cout << "Enter string to decompress: ";
            getline(cin, input);
            string decompressed = rleDecompress(input);
            cout << "Decompressed: " << decompressed << endl;
            break;
        }
        case 3:
        {
            cout << "Enter input file path: ";
            getline(cin, input);
            cout << "Enter output file path: ";
            getline(cin, output);
            compressFile(input, output);
            break;
        }
        case 4:
        {
            cout << "Enter compressed file path: ";
            getline(cin, input);
            cout << "Enter output file path: ";
            getline(cin, output);
            decompressFile(input, output);
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
