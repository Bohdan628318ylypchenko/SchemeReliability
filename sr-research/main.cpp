import research;

import std;
using std::print;

int main()
{
    print("\n=== original s23 ===\n");
    research::s23_original();

    print("\n=== s23 with rt (7 7 7 8 8) ===\n");
    research::s23_rt_7_7_7_8_8();

    print("\n=== s23 with rt and modified connections ===\n");
    research::s23_rt_7_7_7_8_8_modified_connections();

    print("\n=== s24 with d9 right ===\n");
    research::s24_d9_right();

    print("\n=== s25 with d9 d10 right ===\n");
    research::s25_d9_d10_right();

    print("\n=== s27 with d9 d10 c7 right c8 left ===\n");
    research::s27_d9_d10_c7_right_c8_left();

    print("\n=== s29 with d9 d10 c7 right c8 left a4 ===\n");
    research::s29_d9_d10_c7_right_c8_left_a4();

    return 0;
}
