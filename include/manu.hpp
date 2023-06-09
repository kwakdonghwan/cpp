// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 기본 기능을 실행하기 위한 UI 매뉴
// 최초 작성일 : 230520
// 최종 수정일 : 230522

#ifndef MANU_HPP
#define MANU_HPP

#include <iostream>
#include <functional>
#include <atomic>
#include <future>
#include <vector>

#include "threadpool.hpp"
#include "editer.hpp"

using namespace std::literals;

namespace kcpp
{
    // class ThreadPool<>;   // kcpp::threadpool pointer
    // class Edit;           // kcpp::Edit pointer , Control terminal imput
    // class DigitValidator; // kcpp::DigitValidator pointer , Edit class`s validation policy : Digit
    class PopupMenu;
    class LeafMenu;
    class unsupported_operation
    {
    };
    class InValid_Input
    {
    };
    class FailToSetTask
    {
    };
    struct IMenuVisitor // 방문자 패턴 visitor
    {
        virtual void visit(PopupMenu *n) = 0;
        virtual void visit(LeafMenu *n) = 0;
        virtual ~IMenuVisitor() {}
    };

    struct IAcceptor
    {
        virtual void accept(IMenuVisitor *visitor) = 0;
        virtual ~IAcceptor() {}
    };

    class BaseMenu : public IAcceptor
    {
        std::string _title;

    public:
        ThreadPool<> *_tpool = nullptr;

    public:
        BaseMenu(const std::string &t, ThreadPool<> *tp) : _title(t), _tpool(tp) {}
        std::string GetTitle() { return _title; }
        ThreadPool<> *GetThreadPool() { return _tpool; }
        void SetThreadPool(ThreadPool<> *tp) { _tpool = tp; }
        int _classtype = 0;         // 0 is popupMenu, 1 is LeafMenu
        virtual void command() = 0; // commend will be asigned only leaf Manu

        virtual void AddMenu(BaseMenu *) { throw unsupported_operation(); }
        virtual BaseMenu *GetSubManu(int idx) { throw unsupported_operation(); }
        virtual ~BaseMenu() { std::cout << " 기본 매뉴를 종료합니다. " << std::endl; }
    };

    class PopupMenu : public BaseMenu
    {
        std::vector<BaseMenu *> _v;
        Edit *_edit;
        DigitValidator *_validator;

    public:
        PopupMenu(const std::string &t, ThreadPool<> *tp = nullptr) : BaseMenu(t, tp)
        {
            _edit = new Edit();
            _validator = new DigitValidator();
            _edit->setValidate(new DigitValidator());
        }
        // AddMenu, if ThreadPool<> is not set, inherits parent's thread pool.
        void AddMenu(BaseMenu *m) override
        {
            if (m->GetThreadPool() == nullptr)
            {
                m->SetThreadPool(this->GetThreadPool());
            }
            _v.push_back(m);
        }
        void command() override
        {
            while (1)
            {
                system("clear");
                int sz = _v.size();
                int idx = 0;

                for (auto m : _v)
                    std::cout << ++idx << ". " << m->GetTitle() << "\n";
                std::cout << sz + 1 << ". 취소[종료]" << std::endl;
                std::cout << "[항목을 선택하십시오] : ";
                int cmd;
                // std::cin >> cmd;
                cmd = _edit->getDataInt();
                // std::cout << "get : " << cmd << std::endl;
                if (cmd == sz + 1)
                    break;
                if (cmd < 1 || cmd > sz + 1)
                    continue;
                _v[cmd - 1]->command();

                if (_v[cmd - 1]->_classtype == 1) // Leaf Menu. pause
                {
                    std::cout << "\n"
                              << "계속 하시려면 아무키나 입력해주십시오." << std::endl;
                    kcpp::getkey();
                    // if (cmd == 0)
                    //     break;
                }
            }
        }
        void accept(IMenuVisitor *visitor) override
        {
            visitor->visit(this);
            for (auto e : _v)
                e->accept(visitor);
        }

        BaseMenu *GetSubManu(int idx) override
        {
            if (idx < 0 && idx > _v.size())
                throw InValid_Input();
            return _v[idx];
        }
        static PopupMenu *create(const std::string &t, ThreadPool<> *tp = nullptr) { return new PopupMenu(t, tp); }
    };

    class LeafMenu : public BaseMenu
    {
        void SetType() { _classtype = 1; }
        using TASK = std::function<void()>;
        TASK _task = nullptr;

    public:
        template <typename _Func, typename... _BoundArgs>
        LeafMenu(const std::string &t, _Func &&task, _BoundArgs &&...args) : BaseMenu(t, nullptr)
        {
            SetType();
            _task = std::bind(std::forward<_Func>(task), std::forward<_BoundArgs>(args)...);
            std::cout << GetTitle() << " : Set command.." << std::endl;
            if (_task == nullptr)
            {
                throw FailToSetTask();
            }
        }

        LeafMenu(const std::string &title) : BaseMenu(title, nullptr) 
        {
            SetType();
            _task = std::bind(&LeafMenu::implementedFunc,this);
            std::cout << GetTitle() << " : Set command.." << std::endl;
            if (_task == nullptr)
            {
                throw FailToSetTask();
            }
        }

        void command()
        {
            if (_task != nullptr)
            {
                _tpool->AddTask(_task);
            }
        }

        void implementedFunc() 
        {
            std::cout << GetTitle() <<  " : 미구현 입니다. " << std::endl;
        }
        // template <typename _Func, typename... _BoundArgs>
        // void SetCommand(_Func &&task, _BoundArgs &&...args)
        //{
        //     _task = std::bind(std::forward<_Func>(task), std::forward<_BoundArgs>(args)...);
        //     std::cout << GetTitle() << " : Set command.." << std::endl;
        //     if (_task == nullptr)
        //     {
        //         throw FailToSetTask();
        //     }
        // }

        void accept(IMenuVisitor *visitor) override { visitor->visit(this); }
    };

}

#endif // MANU_HPP

// MANU_HPP
//****************************************************************************************//
// example code
// base usange

/*
#include "headers.h"

using namespace kcpp;

void testfunction()
{
    std::cout << "test function call" << std::endl;

}

int main()
{
    ThreadPool tp;
    PopupMenu* root = new PopupMenu("root",&tp);

    root->AddMenu(new LeafMenu("테스트1")); //0
    root->GetSubManu(0)->SetCommand(&testfunction);
    root->AddMenu(new LeafMenu("테스트2")); //1
    root->AddMenu(new PopupMenu("하위 항목 테스트 1")); //2

    root->GetSubManu(2)->AddMenu(new LeafMenu("테스트1"));
    root->GetSubManu(2)->AddMenu(new LeafMenu("테스트2"));
    root->GetSubManu(2)->AddMenu(new LeafMenu("테스트3"));

    root->command();
    std::cout << "기본 프로그램을 종료합니다." <<std::endl;
}
*/