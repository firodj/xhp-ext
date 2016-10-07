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

    rope = code_rope("   ash   of     hol   ");
    rope.htmlTrim();
    cout << "[" << rope.c_str() << "]" << endl;

    rope = code_rope("waht   de    faak");
    rope.htmlTrim();
    cout << "[" << rope.c_str() << "]" << endl;

    rope = code_rope("");
    rope.htmlTrim();
    cout << "[" << rope.c_str() << "]" << endl;

    rope = code_rope("                ");
    rope.htmlTrim();
    cout << "[" << rope.c_str() << "]" << endl;

    rope = code_rope("jika uang sa&#64;ya &lt;-25 mak&ku&#x040;&quot;til;a dia we-&amp;  n kamu diam");

    rope.xhpDecode();

    cout << "decode:" << rope.c_str() << endl;

    cout << endl;
    return 0;
}
