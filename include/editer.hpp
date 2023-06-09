// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 터미널 기반 안정적인 문자열 혹은 숫자 입력을 위한 hpp
// 최초 작성일 : 230520
// 최종 수정일 : 230522

#ifndef EDITOR_HPP
#define EDITOR_HPP
#include <iostream>
#include <string>
#include <curses.h>
#include <stdio.h>
#include "helper.hpp"
namespace kcpp
{

    struct IValidator
    {
        virtual bool validate(const std::string &s, char c) = 0; // 문자열 입력을 확인하는 정책
        virtual bool iscomplete(const std::string &s)
        {
            if (s.compare("") == 0) return false;
            return true;

        } // 완료를 확인하는 정책

        virtual ~IValidator() {}
    };

    class Edit
    {
        std::string data;
        IValidator *val = nullptr;

        // Get data with validation Policy
        void _getData()
        {
            while (1)
            {
                char c = kcpp::getkey();
                // std::cout <<  c ;
                // char c = getch();
                // char c = std::cin.get();
                // std::cout << " \n 다음 입력받음 : " << c;
                //std::cout << "current return ["<<(int)c << "]";
                // if (c == 13 && (val == nullptr || val->iscomplete(data)))
                if((int)c == 127 && data.size() > 0)
                {
                    data.pop_back();
                    std::cout << "\b \b";
                    //std::cout << data;
                    continue;
                    
                }

                if (c == '\n' && (val == nullptr || val->iscomplete(data)))
                    break;

                if (val == nullptr || val->validate(data, c))
                {
                    data.push_back(c);
                    std::cout << c;
                }
            }
            std::cout << "\n";
        }

    public:
        void setValidate(IValidator *p) { val = p; }
        std::string getDataStr()
        {
            data.clear();
            _getData();
            return data;
        }
        int getDataInt()
        {
            data.clear();
            _getData();
            return std::stoi(data);
        }
        double getDataDouble()
        {
            data.clear();
            _getData();
            return std::stod(data);
        }
    };

    // Limit number of input digit
    class LimitDigitValidator : public IValidator
    {
        int limit;

    public:
        LimitDigitValidator(int n) : limit(n) {}

        bool validate(const std::string &s, char c) override
        {
            return s.size() < limit && isdigit(c);
        }

        bool iscomplete(const std::string &s) override
        {
            return s.size() == limit;
        }
    };

    class DigitValidator : public IValidator
    {
    public:
        DigitValidator() {}

        bool validate(const std::string &s, char c) override
        {
            return isdigit(c);
        }

    };

}

#endif // EDITOR_HPP