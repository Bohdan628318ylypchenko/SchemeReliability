import sr_research;

import std;

using std::print;

int main()
{
    //print("\n=== simple ===\n");
    //sr::research::simple();

    print("\n=== original s23 ===\n");
    sr::research::s23_original();

    print("\n=== s23 with rt (7 7 7 8 8) ===\n");
    sr::research::s23_rt_7_7_7_8_8();

    print("\n=== s23 with rt 1: modified connections");
    sr::research::s23_rt_7_7_7_8_8_1_modified_connections();

    return 0;
}
