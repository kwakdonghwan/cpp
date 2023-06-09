// 작성자 : 곽동환
// 이메일 : arbiter1225@gmail.com
// License : GPLv3
// 요약 : 자주 사용하는 매크로 및 문법 정리 
// 최초 작성일 : 230520
// 최종 수정일 : 230522

#ifndef HELPER_HPP
#define HELPER_HPP

// 싱글톤으로 class를 만들어주는 메크로
#define MAKE_SINGLETON(classname)    \
private:                             \
	classname() {}                   \
                                     \
public:                              \
	static classname &getInstance()  \
	{                                \
		static classname instance;   \
		return instance;             \
	}                                \
                                     \
public:                              \
	classname(classname &) = delete; \
	void operator=(classname &) = delete;


#include <termios.h>

namespace kcpp{

int getkey(int is_echo = false) 
{
    int ch;
    struct termios old;
    struct termios current;

    /* 현재 설정된 terminal i/o 값을 backup함 */
    tcgetattr(0, &old);
    
    /* 현재의 설정된 terminal i/o에 일부 속성만 변경하기 위해 복사함 */
    current = old; 

    /* buffer i/o를 중단함 */
    current.c_lflag &= ~ICANON; 
    
    if (is_echo) {  // 입력값을 화면에 표시할 경우
        current.c_lflag |= ECHO; 
    } else {        // 입력값을 화면에 표시하지 않을 경우
        current.c_lflag &= ~ECHO;
    }
    
    /* 변경된 설정값으로 설정합니다.*/
    tcsetattr(0, TCSANOW, &current);
    ch = getchar();
    tcsetattr(0, TCSANOW, &old);

    return ch;
}
}


#endif // HELPER_HPP