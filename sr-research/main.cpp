import sr_research;

import std;

using std::print;

int main()
{
    print("\n=== simple ===\n");
    sr::research::simple();

    print("\n=== original v17 ===\n");
    sr::research::v17_original();

    print("\n=== v17 with medium rt (brute force) ===\n");
    sr::research::v17_rt_medium_brute_force();

    print("\n=== v17 with medium rt (greedy) ===\n");
    sr::research::v17_rt_medium_greedy();

    print("\n=== v17 with large rt (brute force) ===\n");
    sr::research::v17_rt_large_brute_force();

    print("\n=== v17 with large rt (greedy) ===\n");
    sr::research::v17_rt_large_greedy();
}
