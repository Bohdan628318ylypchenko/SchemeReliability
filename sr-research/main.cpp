import sr_research;

import std;

using std::print;

int main()
{
    //print("\n=== simple ===\n");
    //sr::research::simple();

    //print("\n=== original s23 ===\n");
    //sr::research::s23_original();

    print("\n=== s23 with rt (7 7 7 8 8) ===\n");
    sr::research::s23_rt_7_7_7_8_8();

    //print("\n=== s23 with medium rt (greedy) ===\n");
    //sr::research::s23_rt_medium_greedy();

    //print("\n=== s23 with large rt (brute force) ===\n");
    //sr::research::s23_rt_large_brute_force();

    //print("\n=== s23 with large rt (greedy) ===\n");
    //sr::research::s23_rt_large_greedy();

    //print("\n=== s23 with medium rt, pr5 joined to b5 ===\n");
    //sr::research::s23_rt_medium_join_pr5_to_b5();

    //print("\n=== s23 with medium rt, pr5 joined to b5, c4 joined to b1 ===\n");
    //sr::research::s23_rt_medium_join_pr5_to_b5_join_c4_to_b1();
}
