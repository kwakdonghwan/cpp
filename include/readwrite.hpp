// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 데이터 파일들을 저장하고 쓸 수있게 해주는 라이브러리
// 최초 작성일 : 230601
// 최종 수정일 : 230606

#ifndef READ_WRITE
#define READ_WRITE

#include "debug.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <zstd.h>
using namespace std::literals;

namespace kcpp
{

    // File interface 
    class File {
    public:
        virtual void write(const std::string& data) = 0;
        virtual std::string read() = 0;
        virtual std::string getFileType() = 0;
    };

    // File class Base
    class BaseFile : public File {
    private:
        std::string filename;
        std::string fileType;

    public:
        BaseFile(const std::string& filename, const std::string& fileType)
            : filename(filename), fileType(fileType) {}

        void write(const std::string& data) override {
            std::ofstream file(filename);
            if (file.is_open()) {
                file << data;
                file.close();
                PRINT_DEBUG( filename + " 파일 쓰기를 완료하였습니다. ", 3);
            }
        }

        std::string read() override {
            std::string data;
            std::ifstream file(filename);
            if (file.is_open()) {
                file.seekg(0,std::ios::end); 
                int size = file.tellg(); 
                data.resize(size);
                file.seekg(0,std::ios::beg);
                file.read(&data[0],size);
                file.close();
                PRINT_DEBUG( filename + " 파일 읽기를 완료하였습니다. ", 3);
                PRINT_DEBUG( "총 파일 크기 : " + std::to_string(size), 4);
            }
            return data;
        }

        std::string getFileType() override {
            return fileType;
        }
    };

    // zip file decorator
    class CompressedFile : public File {
    private:
        File* file;
    
    public:
        CompressedFile(File* file) : file(file) {}
    
        void write(const std::string& data) override {
            std::string compressedData = compress(data);
            file->write(compressedData);
        }
    
        std::string read() override {
            std::string compressedData = file->read();
            return decompress(compressedData);
        }
    
        std::string getFileType() override {
            return "Compressed " + file->getFileType();
        }
    
    private:
        std::string compress(const std::string& data) {
            PRINT_DEBUG( "데이터를 압축합니다. ", 3);
            return "compressed_" + data;
        }
    
        std::string decompress(const std::string& compressedData) {
            PRINT_DEBUG( "데이터 압축을 해제합니다. ", 3);
            return compressedData.substr(11); 
        }
    };

    // Encrypte decorator 
    class EncryptedFile : public File {
    private:
        File* file;
    
    public:
        EncryptedFile(File* file) : file(file) {}
    
        void write(const std::string& data) override {
            std::string encryptedData = encrypt(data);
            file->write(encryptedData);
        }
    
        std::string read() override {
            std::string encryptedData = file->read();
            return decrypt(encryptedData);
        }
    
        std::string getFileType() override {
            return "Encrypted " + file->getFileType();
        }
    
    private:
        std::string encrypt(const std::string& data) {
            PRINT_DEBUG( "데이터를 암호화합니다. ", 3);
            return "encrypted_" + data;
        }
    
        std::string decrypt(const std::string& encryptedData) {
            PRINT_DEBUG( "데이터를 복호화합니다. ", 3);
            return encryptedData.substr(10); 
        }
    };

    // CSV file read-write decorator 최종단, write_read 숨김
    using CSVFileType = std::vector<std::vector<std::string>>;
    using CSVLineType = std::vector<std::string>;
    class CSVFile : public File {
    private:
        File* file;
    
    public:
        CSVFile(File* file) : file(file) {}

        void writeCSV(const CSVFileType& data) {
            std::string csvdata = writeCsv(data);
            this->write(csvdata);
        }
        void writeAppend(const CSVFileType& data) {
            std::string csvdata = writeCsvAppend(data);
            this->write(csvdata);
        }     
        CSVFileType readCSV() {
            std::string csvdata = file->read();
            return readCsv(csvdata);
        }
    
        std::string getFileType() override {
            return "Csv_" + file->getFileType();
        }
    
    private:
        // This Function Must Be Hidden. 
        void write(const std::string& data) override {
            file->write(data);
        }
    
        std::string read() override {
            //std::string csvdata = file->read();
            return file->read();
        }

        std::string writeCsv(const CSVFileType& data) {
            PRINT_DEBUG( "CSV 쓰기를 수행합니다. ", 3);
            std::stringstream csvedData;
            for(const auto& row : data)
            {
                for(auto it = row.begin(); it != row.end(); ++it)
                {
                    csvedData << *it; 
                    if (std::next(it) != row.end())
                    {
                        csvedData << ",";
                    }
                }
                csvedData << "\n";
            }
            return csvedData.str();
        }
        std::string writeCsvAppend(const CSVFileType& data) {
            PRINT_DEBUG( "CSV 이어쓰기를 수행합니다. ", 3);
            std::stringstream csvedData;
            // Read exsist data
            CSVFileType newData = this->readCSV();
            newData.insert(newData.end(), data.begin(), data.end());
            for(const auto& row : newData)
            {
                for(auto it = row.begin(); it != row.end(); ++it)
                {
                    csvedData << *it; 
                    if (std::next(it) != row.end())
                    {
                        csvedData << ",";
                    }
                }
                csvedData << "\n";
            }
            return csvedData.str();
        }
    
        CSVFileType readCsv(std::string data) {
            PRINT_DEBUG( "CSV 읽기를 수행합니다. ", 3);
            CSVFileType csvdata; 
            std::stringstream _data(data);
            std::string line; 
            while(std::getline(_data,line))
            {
                CSVLineType row; 
                std::stringstream _line(line); 
                std::string cell;

                while(std::getline(_line,cell,','))
                {
                    row.push_back(cell);
                }
                csvdata.push_back(row);
            }

            return csvdata; 
        }
    };

// // example
// int main() {
//     // 기본 파일 객체 생성
//     File* file = new BaseFile("example.txt", "Text File");
// 
//     // 기본 파일에 데이터 쓰기
//     file->write("Hello, World!");
// 
//     // 기본 파일에서 데이터 읽기
//     std::string data = file->read();
//     std::cout << "읽은 데이터: " << data << std::endl;
//     std::cout << "파일 형식: " << file->getFileType() << std::endl;
// 
//     // 파일을 압축하는 데코레이터 생성
//     File* compressedFile = new CompressedFile(file);
// 
//     // 압축된 파일에 데이터 쓰기
//     compressedFile->write("Hello, Decorator Pattern!");
// 
//     // 압축된 파일에서 데이터 읽기
//     std::string compressedData = compressedFile->read();
//     std::cout << "압축 해제된 데이터: " << compressedData << std::endl;
//     std::cout << "파일 형식: " << compressedFile->getFileType() << std::endl;
// 
//     // 파일을 암호화하는 데코레이터 생성
//     File* encryptedFile = new EncryptedFile(compressedFile);
// 
//     // 암호화된 파일에 데이터 쓰기
//     encryptedFile->write("Hello, Encryption!");
// 
//     // 암호화된 파일에서 데이터 읽기
//     std::string decryptedData = encryptedFile->read();
//     std::cout << "복호화된 데이터: " << decryptedData << std::endl;
//     std::cout << "파일 형식: " << encryptedFile->getFileType() << std::endl;
// 
//     delete encryptedFile;
//     delete compressedFile;
//     delete file;
// 
//     return 0;
// }

}

#endif 