#include <iostream>
#include "code_rope.hpp"

using namespace std;

int main(int argc, char* argv[])
{
    code_rope rope = code_rope("x:element-something:wrong");

    cout << "rope-c_str: " << rope.c_str() << endl;
    rope.xhpLabel();

    cout << "rope-xhp-label:" << rope.c_str() << endl;

    rope = code_rope("   ash   of     hol   ");
    rope.htmlTrim();
    cout << "time-everywhere:[" << rope.c_str() << "]" << endl;

    rope = code_rope("waht   de    faak");
    rope.htmlTrim();
    cout << "trim-middle:[" << rope.c_str() << "]" << endl;

    rope = code_rope("");
    rope.htmlTrim();
    cout << "trim-empty:[" << rope.c_str() << "]" << endl;

    rope = code_rope("                ");
    rope.htmlTrim();
    cout << "trim-blank:[" << rope.c_str() << "]" << endl;

    rope = code_rope("'besok ja'n'g'an nakalz'");
    rope.squote_escape();
    cout << "squote:" << rope.c_str() << endl;

    rope = code_rope("jika uang&reg; sa&#64;ya &lt;-25 mak&ku&#x040;&quot;til;a dia we-&amp; &#0f;n &middot;kamu' 'diam &raquo;.");
    rope.xhpDecode();

    cout << "decode:" << rope.c_str() << endl;

    return 0;
}
