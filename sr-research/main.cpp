import sr_research;

import std;

using std::println;

int main()
{
    println("=== simple ===\n");
    sr::research::simple();

    println("=== original v17 ===");
    sr::research::v17_original();

    println("=== v17 with rt ===");
    sr::research::v17_rt();
}
