#include <iostream>
#include "code_rope.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    code_rope rope = code_rope("x:element-something:wrong");

    cout << "rope = " << rope.c_str() << endl;

    rope.xhpLabel();

    cout << "rope = " << rope.c_str() << endl;

    cout << "test = " << (rope == "\\xhp_x__element_something__wrong") << endl;

    cout << endl;
    return 0;
}
